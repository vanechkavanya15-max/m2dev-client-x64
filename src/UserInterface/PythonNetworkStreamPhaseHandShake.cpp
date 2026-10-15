#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "PythonApplication.h"
#include "Packet.h"
#include "Client/Network/Protocol/LuriaProtocol.h"
#include "Client/Network/Protocol/BeaviumProtocol.h"
#include "EterLib/BlowfishCryptoProvider.h"
#include "EterLib/SodiumCryptoProvider.h"

// HandShake ---------------------------------------------------------------------------
void CPythonNetworkStream::HandShakePhase()
{
	DispatchPacket(m_handshakeHandlers);
}

void CPythonNetworkStream::SetHandShakePhase()
{
	if ("HandShake"!=m_strPhase)
		m_phaseLeaveFunc.Run();

	Tracef("[PHASE] Entering phase: HandShake\n");
	Tracen("");
	Tracen("## Network - Hand Shake Phase ##");
	Tracen("");

	m_strPhase = "HandShake";
	RefreshProtocolHandlers();

	m_dwChangingPhaseTime = ELTimer_GetMSec();
	m_phaseProcessFunc.Set(this, &CPythonNetworkStream::HandShakePhase);
	m_phaseLeaveFunc.Set(this, &CPythonNetworkStream::__LeaveHandshakePhase);

	SetGameOnline();

	if (__DirectEnterMode_IsSet())
	{
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOGIN], "OnHandShake", Py_BuildValue("()"));
	}
}

// Override: extract time sync before delegating crypto to base class
bool CPythonNetworkStream::RecvKeyChallenge()
{
	// Peek to extract server_time before base class Recv() consumes the packet
	TPacketGCKeyChallenge packet;
	if (!Peek(sizeof(packet), &packet))
		return false;

	// Time sync from server (game connection only)
	m_kServerTimeSync.m_dwChangeServerTime = packet.server_time;
	m_kServerTimeSync.m_dwChangeClientTime = ELTimer_GetMSec();
	ELTimer_SetServerMSec(packet.server_time);
	CTimer::Instance().SetBaseTime();

	Tracef("[HANDSHAKE] KeyChallenge: server_time=%u, client_time=%u, offset=%d\n",
		packet.server_time, ELTimer_GetMSec(), (int)(packet.server_time - ELTimer_GetMSec()));

	// Base class handles cipher init, key computation, and response
	return CNetworkStream::RecvKeyChallenge();
}

// RecvKeyComplete: no game-specific additions, delegate entirely to base
bool CPythonNetworkStream::RecvKeyComplete()
{
	return CNetworkStream::RecvKeyComplete();
}

// ========================================================================
// Luria Handshake Handlers (0xFF, 0xFB, 0xFA, 0xFD)
// ========================================================================

bool CPythonNetworkStream::RecvPhasePacketLuria()
{
	uint8_t header = 0;
	uint8_t phase = 0;
	if (!Recv(sizeof(header), &header) || !Recv(sizeof(phase), &phase))
		return false;

	Tracef("[PHASE] Luria game connection phase changed to: %u\n", phase);

	switch (phase)
	{
		case 1: // Handshake
			SetHandShakePhase();
			break;
		case 2: // Login
			SetLoginPhase();
			break;
		case 3: // Select
			SetSelectPhase();
			break;
		case 4: // Loading
			SetLoadingPhase();
			break;
		case 5: // Game
			SetGamePhase();
			break;
		case 0: // Close
			ClosePhase();
			break;
		default:
			break;
	}
	return true;
}

bool CPythonNetworkStream::RecvHandshakePacketLuria()
{
	Luria::TPacketCGHandshakeLuria hsPacket{};
	if (!Recv(sizeof(hsPacket), &hsPacket))
		return false;

	m_kServerTimeSync.m_dwChangeServerTime = hsPacket.dwTime;
	m_kServerTimeSync.m_dwChangeClientTime = ELTimer_GetMSec();
	ELTimer_SetServerMSec(hsPacket.dwTime);
	CTimer::Instance().SetBaseTime();

	Luria::TPacketCGHandshakeLuria ackPacket{};
	ackPacket.header = Luria::CG::HANDSHAKE;
	ackPacket.dwHandshake = hsPacket.dwHandshake;
	ackPacket.dwTime = hsPacket.dwTime + hsPacket.lDelta * 2;
	ackPacket.lDelta = 0;

	if (!Send(sizeof(ackPacket), &ackPacket))
	{
		TraceError("CPythonNetworkStream - Failed to send Luria Handshake ACK");
		return false;
	}
	return true;
}

bool CPythonNetworkStream::RecvKeyAgreementPacketLuria()
{
	uint8_t header = 0;
	uint16_t wType = 0;
	uint16_t wLength = 0;
	if (!Recv(sizeof(header), &header))
		return false;
	if (!Recv(sizeof(wType), &wType))
		return false;
	if (!Recv(sizeof(wLength), &wLength))
		return false;

	if (wLength != 256)
	{
		TraceError("CPythonNetworkStream - Invalid Luria KeyAgreement length: %u", wLength);
		return false;
	}

	uint8_t sPubData[256];
	if (!Recv(sizeof(sPubData), sPubData))
		return false;

	Luria::LuriaDH2Keys keys{};
	if (!Luria::NegotiateDH2Keys(std::span<const uint8_t>(sPubData, 256), keys))
	{
		TraceError("CPythonNetworkStream - Luria DH2 negotiation failed");
		return false;
	}

	m_luriaDH2Keys = keys;

	Luria::TPacketCGKeyAgreementLuria cgPkt{};
	cgPkt.header = Luria::CG::KEY_AGREEMENT;
	cgPkt.wType = wType;
	cgPkt.wLength = 256;
	std::memcpy(cgPkt.abData, keys.cgPubData.data(), 256);

	if (!Send(sizeof(cgPkt), &cgPkt))
	{
		TraceError("CPythonNetworkStream - Failed to send Luria KeyAgreement CG");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::RecvKeyAgreementAckLuria()
{
	uint8_t ackBuf[4];
	if (!Recv(sizeof(ackBuf), ackBuf))
		return false;

	if (ackBuf[0] != Luria::GC::KEY_AGREEMENT_ACK)
	{
		TraceError("CPythonNetworkStream - Expected 0xFA, got 0x%02X", ackBuf[0]);
		return false;
	}

	auto blowfish = std::make_shared<BlowfishCryptoProvider>();
	blowfish->SetKeys(m_luriaDH2Keys.key2, m_luriaDH2Keys.iv2,
	                  m_luriaDH2Keys.key1, m_luriaDH2Keys.iv1);
	blowfish->SetActivated(true);
	SetCryptoProvider(blowfish);
	DecryptPendingRecvData();

	Tracef("[HANDSHAKE] Luria Game crypto activated (Blowfish CTR).\n");
	return true;
}

// ========================================================================
// Beavium Handshake Handlers (0xFF, 0xFB, 0xFA)
// ========================================================================

bool CPythonNetworkStream::RecvHandshakePacketBeavium()
{
	Beavium::TPacketGCHandshakeBeavium hsPacket{};
	if (!Recv(sizeof(hsPacket), &hsPacket))
		return false;

	m_kServerTimeSync.m_dwChangeServerTime = hsPacket.srvTime;
	m_kServerTimeSync.m_dwChangeClientTime = ELTimer_GetMSec();
	ELTimer_SetServerMSec(hsPacket.srvTime);
	CTimer::Instance().SetBaseTime();

	Tracef("[HANDSHAKE] Beavium greeting: token=0x%08X, srvTime=%u, delta=%u\n",
		hsPacket.token, hsPacket.srvTime, hsPacket.delta);

	Beavium::TPacketCGHandshakeAckBeavium ackPacket{};
	ackPacket.header = Beavium::CG::HANDSHAKE_ACK; // 0xFF
	ackPacket.token = hsPacket.token;
	Beavium::CalculateHandshakeSignatures(hsPacket.token, hsPacket.srvTime, ackPacket.sig1, ackPacket.sig2);

	if (!Send(sizeof(ackPacket), &ackPacket))
	{
		TraceError("CPythonNetworkStream - Failed to send Beavium Handshake ACK");
		return false;
	}

	Tracef("[HANDSHAKE] Beavium Handshake ACK sent (sig1=0x%08X, sig2=0x%08X)\n",
		ackPacket.sig1, ackPacket.sig2);

	return true;
}

bool CPythonNetworkStream::RecvKeyAgreementPacketBeavium()
{
	uint8_t offerBuf[137]{};
	if (!Recv(sizeof(offerBuf), offerBuf))
		return false;

	const uint8_t* serverPub = &offerBuf[1];

	auto sodiumProvider = std::make_shared<SodiumCryptoProvider>();
	if (!sodiumProvider->GetCipher().Initialize())
	{
		TraceError("CPythonNetworkStream - Failed to initialize SodiumCryptoProvider cipher");
		return false;
	}

	uint8_t clientPub[32]{};
	sodiumProvider->GetCipher().GetPublicKey(clientPub);

	if (!sodiumProvider->GetCipher().ComputeClientKeys(serverPub))
	{
		TraceError("CPythonNetworkStream - Failed to compute Beavium X25519 client keys");
		return false;
	}

	uint8_t ackBuf[137]{};
	ackBuf[0] = Beavium::CG::KEY_AGREEMENT; // 0xFA
	std::memcpy(&ackBuf[1], clientPub, 32);
	std::memcpy(&ackBuf[33], &offerBuf[33], 32);

	if (!Send(sizeof(ackBuf), ackBuf))
	{
		TraceError("CPythonNetworkStream - Failed to send Beavium KeyAgreement ACK");
		return false;
	}

	SetCryptoProvider(sodiumProvider);
	Tracef("[HANDSHAKE] Beavium KeyAgreement complete, SodiumCryptoProvider registered.\n");
	return true;
}

bool CPythonNetworkStream::RecvKeyAgreementAckBeavium()
{
	uint8_t ackBuf[4]{};
	if (!Recv(sizeof(ackBuf), ackBuf))
		return false;

	if (ackBuf[0] != Beavium::GC::KEY_AGREEMENT_ACK)
	{
		TraceError("CPythonNetworkStream - Expected 0xFA KeyAgreementAck, got 0x%02X", ackBuf[0]);
		return false;
	}

	auto crypto = GetCryptoProvider();
	if (crypto)
	{
		crypto->SetActivated(true);
		DecryptPendingRecvData();
		Tracef("[HANDSHAKE] Beavium Game crypto activated (Sodium XChaCha20-Poly1305).\n");
	}

	return true;
}

