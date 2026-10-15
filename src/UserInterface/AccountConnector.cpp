#include "StdAfx.h"
#include "AccountConnector.h"
#include "Packet.h"
#include "PythonNetworkStream.h"
#include "Client/Network/Protocol/ProtocolDriverRegistry.h"
#include "Client/Network/Protocol/LuriaProtocol.h"
#include "Client/Network/Protocol/BeaviumProtocol.h"
#include "EterLib/BlowfishCryptoProvider.h"
#include "EterLib/SodiumCryptoProvider.h"
#include "FastBootOverlay.h"

void CAccountConnector::SetHandler(PyObject* poHandler)
{
	m_poHandler = poHandler;
}

void CAccountConnector::SetLoginInfo(const char * c_szName, const char * c_szPwd, const char * c_szPin)
{
	m_strID = c_szName ? c_szName : "";
	m_strPassword = c_szPwd ? c_szPwd : "";
	m_strPin = c_szPin ? c_szPin : "";
}


void CAccountConnector::ClearLoginInfo( void )
{
	m_strPassword = "";
	m_strPin = "";
}

bool CAccountConnector::Connect(const char * c_szAddr, int iPort, const char * c_szAccountAddr, int iAccountPort)
{
	m_strAddr = c_szAddr ? c_szAddr : "";
	m_iPort = iPort;
	__OfflineState_Set();

	if (iAccountPort == 11469 || (c_szAccountAddr && std::string_view(c_szAccountAddr).find("46.105.73.82") != std::string_view::npos))
	{
		Network::Protocol::ProtocolDriverRegistry::Instance().SetActiveDriver("luria");
	}

	return CNetworkStream::Connect(c_szAccountAddr ? c_szAccountAddr : "", iAccountPort);
}

void CAccountConnector::Disconnect()
{
	CNetworkStream::Disconnect();
	__OfflineState_Set();
}

void CAccountConnector::Process()
{
	CNetworkStream::Process();

	if (!__StateProcess())
	{
		__OfflineState_Set();
		Disconnect();
	}
}

bool CAccountConnector::__StateProcess()
{
	switch (m_eState)
	{
		case STATE_HANDSHAKE:
			return __HandshakeState_Process();
			break;
		case STATE_AUTH:
			return __AuthState_Process();
			break;
	}

	return true;
}

bool CAccountConnector::__HandshakeState_Process()
{
	int nAvail = GetRecvBufferSize();
	if (nAvail <= 0)
		return true;

	auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	if (!pDriver)
	{
		Network::Protocol::ProtocolDriverRegistry::Instance().InitializeDefaults();
		pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	}

	int peekLen = std::min(nAvail, 64);
	std::vector<uint8_t> peekBuffer(peekLen);
	if (!Peek(peekLen, peekBuffer.data()))
		return true;

	// Odrzucenie ewentualnego zerowego wyrownania
	size_t zeroCount = 0;
	while (zeroCount < peekBuffer.size() && peekBuffer[zeroCount] == 0)
		zeroCount++;
	if (zeroCount > 0)
	{
		Recv(static_cast<int>(zeroCount));
		return true;
	}

	std::span<const uint8_t> peekSpan(peekBuffer.data(), peekBuffer.size());
	auto frameOpt = pDriver ? pDriver->InspectFrame(peekSpan) : std::nullopt;
	if (frameOpt.has_value())
	{
		const auto& frame = frameOpt.value();
		if (nAvail < static_cast<int>(frame.packetLength))
			return true; // Oczekiwanie na pelny bufor ramki z gniazda TCP

		uint16_t header = frame.unifiedOpcode;
		if (pDriver->IsHandshakeOpcode(header))
		{
			std::vector<uint8_t> packetBuffer(frame.packetLength);
			if (!Recv(frame.packetLength, packetBuffer.data()))
				return false;

			std::span<const uint8_t> packetSpan(packetBuffer.data(), packetBuffer.size());
			return pDriver->HandleHandshakePacket(header, packetSpan, *this);
		}
	}

	// Obsluga przejscia fazy oraz ping
	if (pDriver && pDriver->GetDriverName() == "luria")
	{
		if (!__AnalyzePacket(Luria::GC::PHASE, 2, &CAccountConnector::__HandshakeState_RecvPhaseLuria))
			return false;
	}
	else
	{
		if (!__AnalyzePacket(GC::PHASE, sizeof(TPacketGCPhase), &CAccountConnector::__AuthState_RecvPhase))
			return false;

		if (!__AnalyzePacket(GC::PING, sizeof(TPacketGCPing), &CAccountConnector::__AuthState_RecvPingBase))
			return false;
	}

	return true;
}

bool CAccountConnector::__AuthState_Process()
{
	if (!__AnalyzePacket(0, sizeof(BYTE), &CAccountConnector::__AuthState_RecvEmpty))
		return true;

	auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	if (pDriver && pDriver->GetDriverName() == "luria")
	{
		if (!__AnalyzePacket(Luria::GC::PHASE, 2, &CAccountConnector::__AuthState_RecvPhaseLuria))
			return false;

		if (!__AnalyzePacket(Luria::GC::AUTH_SUCCESS, sizeof(Luria::TPacketGCAuthSuccessLuria), &CAccountConnector::__AuthState_RecvAuthSuccessLuria))
			return true;

		if (!__AnalyzePacket(Luria::GC::LOGIN_FAILURE, 10, &CAccountConnector::__AuthState_RecvLoginFailureLuria))
			return true;

		return true;
	}

	if (!__AnalyzePacket(GC::PHASE, sizeof(TPacketGCPhase), &CAccountConnector::__AuthState_RecvPhase))
		return false;

	if (!__AnalyzePacket(GC::PING, sizeof(TPacketGCPing), &CAccountConnector::__AuthState_RecvPingBase))
		return false;

	if (!__AnalyzePacket(GC::AUTH_SUCCESS, sizeof(TPacketGCAuthSuccess), &CAccountConnector::__AuthState_RecvAuthSuccess))
		return true;

	if (!__AnalyzePacket(GC::LOGIN_FAILURE, sizeof(TPacketGCLoginFailure), &CAccountConnector::__AuthState_RecvAuthFailure))
		return true;

	if (!__AnalyzePacket(GC::KEY_CHALLENGE, sizeof(TPacketGCKeyChallenge), &CAccountConnector::__AuthState_RecvKeyChallengeBase))
		return false;

	if (!__AnalyzePacket(GC::KEY_COMPLETE, sizeof(TPacketGCKeyComplete), &CAccountConnector::__AuthState_RecvKeyCompleteBase))
		return false;

	return true;
}

bool CAccountConnector::__AuthState_RecvEmpty()
{
	BYTE byEmpty;
	Recv(sizeof(BYTE), &byEmpty);
	return true;
}

bool CAccountConnector::__AuthState_RecvPhase()
{
	TPacketGCPhase kPacketPhase;
	if (!Recv(sizeof(kPacketPhase), &kPacketPhase))
		return false;

	if (kPacketPhase.phase == PHASE_HANDSHAKE)
	{
		__HandshakeState_Set();
	}
	else if (kPacketPhase.phase == PHASE_AUTH)
	{
		auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
		if (!pDriver)
		{
			Network::Protocol::ProtocolDriverRegistry::Instance().InitializeDefaults();
			pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
		}

		auto authRes = pDriver->EncodeAuthLogin(m_strID, m_strPassword, m_strPin);
		if (!authRes.has_value())
		{
			Tracen(" CAccountConnector::__AuthState_RecvPhase - EncodeAuthLogin failed");
			return false;
		}

		const auto& authData = authRes.value();
		if (!Send(static_cast<int>(authData.size()), authData.data()))
		{
			Tracen(" CAccountConnector::__AuthState_RecvPhase - Send Auth Error");
			return false;
		}
		__SendInternalBuffer();

		m_isWaitKey = TRUE;
		__AuthState_Set();
	}

	return true;
}

// Key exchange (RecvKeyChallenge, RecvKeyComplete) and ping/pong (RecvPingPacket)
// are now handled by CNetworkStream base class. Thin wrappers in the header
// delegate __AnalyzePacket dispatch to those base methods.

bool CAccountConnector::__AuthState_RecvAuthSuccess()
{
	TPacketGCAuthSuccess kAuthSuccessPacket;
	if (!Recv(sizeof(kAuthSuccessPacket), &kAuthSuccessPacket))
		return false;

	TraceError("[AUTH] Odebrano AUTH_SUCCESS: dwLoginKey=%u, bResult=%u", kAuthSuccessPacket.dwLoginKey, (unsigned int)kAuthSuccessPacket.bResult);

	m_isWaitKey = FALSE;
	if (!kAuthSuccessPacket.bResult)
	{
		UserInterface::FastBoot::FastBootOverlay::Instance().SetLoginFailure("BESAMEKEY");
		if (m_poHandler)
			PyCallClassMemberFunc(m_poHandler, "OnLoginFailure", Py_BuildValue("(s)", "BESAMEKEY"));
	}
	else
	{
		CPythonNetworkStream & rkNet = CPythonNetworkStream::Instance();
		rkNet.SetLoginKey(kAuthSuccessPacket.dwLoginKey);
		rkNet.Connect(m_strAddr.c_str(), m_iPort);
	}

	Disconnect();
	__OfflineState_Set();

	return true;
}

bool CAccountConnector::__AuthState_RecvAuthFailure()
{
	TPacketGCLoginFailure packet_failure;
	if (!Recv(sizeof(TPacketGCLoginFailure), &packet_failure))
		return false;

	UserInterface::FastBoot::FastBootOverlay::Instance().SetLoginFailure(packet_failure.szStatus);
	if (m_poHandler)
		PyCallClassMemberFunc(m_poHandler, "OnLoginFailure", Py_BuildValue("(s)", packet_failure.szStatus));

	Disconnect();
	__OfflineState_Set();
	return true;
}

bool CAccountConnector::__HandshakeState_RecvPhaseLuria()
{
	uint8_t header = 0;
	uint8_t phase = 0;
	if (!Recv(sizeof(header), &header) || !Recv(sizeof(phase), &phase))
		return false;

	TraceError("[HANDSHAKE_LURIA] Odebrano pakiet PHASE 0xFD: phase=%u", (unsigned int)phase);

	if (phase == 1) // PHASE_HANDSHAKE
	{
		__HandshakeState_Set();
	}
	else if (phase == 10) // PHASE_AUTH
	{
		__AuthState_Set();
		if (!m_bLuriaAuthSent)
		{
			__SendAuthPacketLuria();
		}
	}
	return true;
}

bool CAccountConnector::__HandshakeState_RecvHandshakeLuria()
{
	Luria::TPacketCGHandshakeLuria hsPacket{};
	if (!Recv(sizeof(hsPacket), &hsPacket))
		return false;

	// Odpowiedz: CG_HANDSHAKE (0xFF, seed, time + delta * 2, 0)
	Luria::TPacketCGHandshakeLuria ackPacket{};
	ackPacket.header = Luria::CG::HANDSHAKE;
	ackPacket.dwHandshake = hsPacket.dwHandshake;
	ackPacket.dwTime = hsPacket.dwTime + hsPacket.lDelta * 2;
	ackPacket.lDelta = 0;

	if (!Send(sizeof(ackPacket), &ackPacket))
	{
		TraceError("CAccountConnector - Failed to send Luria Handshake ACK");
		return false;
	}
	return true;
}

bool CAccountConnector::__HandshakeState_RecvKeyAgreementLuria()
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
		TraceError("CAccountConnector - Invalid Luria KeyAgreement length: %u", wLength);
		return false;
	}

	uint8_t sPubData[256];
	if (!Recv(sizeof(sPubData), sPubData))
		return false;

	Luria::LuriaDH2Keys keys{};
	if (!Luria::NegotiateDH2Keys(std::span<const uint8_t>(sPubData, 256), keys))
	{
		TraceError("CAccountConnector - Luria DH2 negotiation failed");
		return false;
	}

	m_luriaDH2Keys = keys;

	// Odeslanie klucza publicznego klienta 0xFB (261 B)
	Luria::TPacketCGKeyAgreementLuria cgPkt{};
	cgPkt.header = Luria::CG::KEY_AGREEMENT;
	cgPkt.wType = wType;
	cgPkt.wLength = 256;
	std::memcpy(cgPkt.abData, keys.cgPubData.data(), 256);

	if (!Send(sizeof(cgPkt), &cgPkt))
	{
		TraceError("CAccountConnector - Failed to send Luria KeyAgreement CG");
		return false;
	}

	return true;
}

bool CAccountConnector::__HandshakeState_RecvKeyAgreementAckLuria()
{
	uint8_t ackBuf[4];
	if (!Recv(sizeof(ackBuf), ackBuf))
		return false;

	if (ackBuf[0] != Luria::GC::KEY_AGREEMENT_ACK)
	{
		TraceError("CAccountConnector - Expected 0xFA, got 0x%02X", ackBuf[0]);
		return false;
	}

	// Aktywacja Blowfish CTR
	auto blowfish = std::make_shared<BlowfishCryptoProvider>();
	blowfish->SetKeys(m_luriaDH2Keys.key2, m_luriaDH2Keys.iv2,
	                  m_luriaDH2Keys.key1, m_luriaDH2Keys.iv1);
	blowfish->SetActivated(true);
	SetCryptoProvider(blowfish);
	DecryptPendingRecvData();

	TraceError("[AUTH_LURIA] KeyAgreementAck 0xFA zaakceptowany, Blowfish CTR aktywny");

	__AuthState_Set();

	// Natychmiastowe wyslanie pakietu autoryzacji 168B po uzgodnieniu kluczy DH2
	__SendAuthPacketLuria();

	return true;
}

bool CAccountConnector::__SendAuthPacketLuria()
{
	if (m_bLuriaAuthSent)
		return true;

	if (m_strID.empty())
	{
		TraceError("[AUTH_LURIA] Blad: pusty login!");
		return false;
	}

	Luria::TPacketCGAuthLuria authPacket{};
	authPacket.header = Luria::CG::AUTH_LOGIN; // 0x6F
	strncpy(authPacket.szLogin, m_strID.c_str(), sizeof(authPacket.szLogin) - 1);
	strncpy(authPacket.szPassword, m_strPassword.c_str(), sizeof(authPacket.szPassword) - 1);
	strncpy(authPacket.szPin, m_strPin.c_str(), sizeof(authPacket.szPin) - 1);
	strncpy(authPacket.szToken, "STARRS3543S22", sizeof(authPacket.szToken) - 1);
	authPacket.adwClientKeys[0] = 1;
	authPacket.adwClientKeys[1] = 2;
	authPacket.adwClientKeys[2] = 3;
	authPacket.adwClientKeys[3] = 4;
	std::memcpy(authPacket.szHwidSha256, "da0cb75f0b7f841dd089e7af0e074add76c810e8f9742b892268157bbab0bc47", 64);
	authPacket.bNullTerminator = 0;

	if (!Send(sizeof(authPacket), &authPacket))
	{
		TraceError("CAccountConnector - Failed to send Luria Auth 0x6F");
		return false;
	}

	m_bLuriaAuthSent = true;
	__SendInternalBuffer();
	TraceError("[AUTH_LURIA] Wyslano pakiet 0x6F (168B) dla loginu: %s", authPacket.szLogin);
	return true;
}

bool CAccountConnector::__AuthState_RecvPhaseLuria()
{
	uint8_t header = 0;
	uint8_t phase = 0;
	if (!Recv(sizeof(header), &header) || !Recv(sizeof(phase), &phase))
		return false;

	TraceError("[AUTH_LURIA] Odebrano pakiet PHASE 0xFD: phase=%u", (unsigned int)phase);

	if (phase == static_cast<uint8_t>(Luria::Phase::Auth)) // 10
	{
		if (!m_bLuriaAuthSent)
		{
			__SendAuthPacketLuria();
		}
	}
	return true;
}

bool CAccountConnector::__AuthState_RecvAuthSuccessLuria()
{
	Luria::TPacketGCAuthSuccessLuria kAuthSuccessPacket{};
	if (!Recv(sizeof(kAuthSuccessPacket), &kAuthSuccessPacket))
		return false;

	TraceError("[AUTH_LURIA] Odebrano AUTH_SUCCESS 0x96: dwLoginKey=%u, bResult=%u",
		kAuthSuccessPacket.dwLoginKey, (unsigned int)kAuthSuccessPacket.bResult);

	if (kAuthSuccessPacket.dwLoginKey == 0 && !kAuthSuccessPacket.bResult)
	{
		UserInterface::FastBoot::FastBootOverlay::Instance().SetLoginFailure("BESAMEKEY");
		if (m_poHandler)
			PyCallClassMemberFunc(m_poHandler, "OnLoginFailure", Py_BuildValue("(s)", "BESAMEKEY"));
	}
	else
	{
		CPythonNetworkStream & rkNet = CPythonNetworkStream::Instance();
		rkNet.SetLoginKey(kAuthSuccessPacket.dwLoginKey);
		TraceError("[AUTH_LURIA] Przelaczanie na Game Server %s:%d (LoginKey=%u)",
			m_strAddr.c_str(), m_iPort, kAuthSuccessPacket.dwLoginKey);
		rkNet.Connect(m_strAddr.c_str(), m_iPort);
	}

	Disconnect();
	__OfflineState_Set();

	return true;
}

bool CAccountConnector::__AuthState_RecvLoginFailureLuria()
{
	uint8_t header = 0;
	char szStatus[9]{};
	if (!Recv(sizeof(header), &header))
		return false;
	if (!Recv(8, szStatus))
		return false;
	uint8_t dummy = 0;
	Recv(sizeof(dummy), &dummy); // Pobranie 10-tego bajtu pakietu

	TraceError("[AUTH_LURIA] Odebrano LOGIN_FAILURE 0x07: szStatus='%s', dummy=0x%02X", szStatus, (unsigned int)dummy);

	UserInterface::FastBoot::FastBootOverlay::Instance().SetLoginFailure(szStatus);

	if (m_poHandler)
		PyCallClassMemberFunc(m_poHandler, "OnLoginFailure", Py_BuildValue("(s)", szStatus));

	Disconnect();
	__OfflineState_Set();
	return true;
}

// ========================================================================
// Beavium specific handshake and auth methods
// ========================================================================

bool CAccountConnector::__HandshakeState_RecvHandshakeBeavium()
{
	Beavium::TPacketGCHandshakeBeavium hsPacket{};
	if (!Recv(sizeof(hsPacket), &hsPacket))
		return false;

	Beavium::TPacketCGHandshakeAckBeavium ackPacket{};
	ackPacket.header = Beavium::CG::HANDSHAKE_ACK; // 0xFF
	ackPacket.token = hsPacket.token;
	Beavium::CalculateHandshakeSignatures(hsPacket.token, hsPacket.srvTime, ackPacket.sig1, ackPacket.sig2);

	if (!Send(sizeof(ackPacket), &ackPacket))
	{
		Tracen(" CAccountConnector - Failed to send Beavium Handshake ACK");
		return false;
	}
	return true;
}

bool CAccountConnector::__HandshakeState_RecvKeyAgreementBeavium()
{
	uint8_t offerBuf[137]{};
	if (!Recv(sizeof(offerBuf), offerBuf))
		return false;

	const uint8_t* serverPub = &offerBuf[1];

	auto sodiumProvider = std::make_shared<SodiumCryptoProvider>();
	if (!sodiumProvider->GetCipher().Initialize())
	{
		Tracen(" CAccountConnector - Failed to initialize SodiumCryptoProvider cipher");
		return false;
	}

	uint8_t clientPub[32]{};
	sodiumProvider->GetCipher().GetPublicKey(clientPub);

	if (!sodiumProvider->GetCipher().ComputeClientKeys(serverPub))
	{
		Tracen(" CAccountConnector - Failed to compute Beavium X25519 client keys");
		return false;
	}

	uint8_t ackBuf[137]{};
	ackBuf[0] = Beavium::CG::KEY_AGREEMENT; // 0xFA
	std::memcpy(&ackBuf[1], clientPub, 32);
	std::memcpy(&ackBuf[33], &offerBuf[33], 32);

	if (!Send(sizeof(ackBuf), ackBuf))
	{
		Tracen(" CAccountConnector - Failed to send Beavium KeyAgreement ACK");
		return false;
	}

	SetCryptoProvider(sodiumProvider);
	return true;
}

bool CAccountConnector::__HandshakeState_RecvKeyAgreementAckBeavium()
{
	uint8_t ackBuf[4]{};
	if (!Recv(sizeof(ackBuf), ackBuf))
		return false;

	if (ackBuf[0] != Beavium::GC::KEY_AGREEMENT_ACK)
	{
		Tracen(" CAccountConnector - Expected 0xFA KeyAgreementAck");
		return false;
	}

	auto crypto = GetCryptoProvider();
	if (crypto)
	{
		crypto->SetActivated(true);
		DecryptPendingRecvData();
	}

	return true;
}


bool CAccountConnector::__AnalyzePacket(UINT uHeader, UINT uPacketSize, bool (CAccountConnector::*pfnDispatchPacket)())
{
	auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	if (!pDriver)
	{
		Network::Protocol::ProtocolDriverRegistry::Instance().InitializeDefaults();
		pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	}

	bool is1ByteHeader = false;
	if (pDriver)
	{
		auto name = pDriver->GetDriverName();
		if (name == "luria" || name == "martysama" || name == "beavium")
			is1ByteHeader = true;
	}

	if (is1ByteHeader)
	{
		uint8_t bHeader = 0;
		if (!Peek(sizeof(bHeader), &bHeader))
			return true;

		if (bHeader != static_cast<uint8_t>(uHeader))
			return true;

		if (!Peek(uPacketSize))
			return true;

		return (this->*pfnDispatchPacket)();
	}
	else
	{
		uint16_t wHeader = 0;
		if (!Peek(sizeof(wHeader), &wHeader))
			return true;

		if (wHeader != static_cast<uint16_t>(uHeader))
			return true;

		if (!Peek(uPacketSize))
			return true;

		return (this->*pfnDispatchPacket)();
	}
}

void CAccountConnector::__OfflineState_Set()
{
	__Inialize();
}

void CAccountConnector::__HandshakeState_Set()
{
	m_eState=STATE_HANDSHAKE;
}

void CAccountConnector::__AuthState_Set()
{
	m_eState=STATE_AUTH;
}

void CAccountConnector::OnConnectFailure()
{
	TraceError("[AUTH] CAccountConnector::OnConnectFailure called");
	UserInterface::FastBoot::FastBootOverlay::Instance().SetLoginFailure("CONNECT_FAIL");

	if (m_poHandler)
		PyCallClassMemberFunc(m_poHandler, "OnConnectFailure", Py_BuildValue("()"));

	__OfflineState_Set();
}

void CAccountConnector::OnConnectSuccess()
{
	m_eState = STATE_HANDSHAKE;
}

void CAccountConnector::OnRemoteDisconnect()
{
	TraceError("[AUTH] CAccountConnector::OnRemoteDisconnect called (m_isWaitKey=%d)", (int)m_isWaitKey);
	if (m_isWaitKey)
	{
		if (m_poHandler)
		{
			PyCallClassMemberFunc(m_poHandler, "OnExit", Py_BuildValue("()"));
			return;
		}
	}

	UserInterface::FastBoot::FastBootOverlay::Instance().SetLoginFailure("SHUTDOWN");

	if (m_poHandler)
	{
		PyCallClassMemberFunc(m_poHandler, "OnLoginFailure", Py_BuildValue("(s)", "SHUTDOWN"));
	}

	__OfflineState_Set();
}

void CAccountConnector::OnDisconnect()
{
	__OfflineState_Set();
}

void CAccountConnector::__Inialize()
{
	m_eState = STATE_OFFLINE;
	m_isWaitKey = FALSE;
	m_bLuriaAuthSent = false;
}

CAccountConnector::CAccountConnector()
{
	m_poHandler = NULL;
	m_strAddr = "";
	m_iPort = 0;

	SetLoginInfo("", "");
	SetRecvBufferSize(1024 * 128);
	SetSendBufferSize(2048);
	__Inialize();
}

CAccountConnector::~CAccountConnector()
{
	__OfflineState_Set();
}
