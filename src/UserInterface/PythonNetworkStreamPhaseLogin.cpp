#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "Packet.h"
#include "AccountConnector.h"
#include "Client/Network/Protocol/ProtocolDriverRegistry.h"
#include "FastBootOverlay.h"

// Login ---------------------------------------------------------------------------
void CPythonNetworkStream::LoginPhase()
{
	DispatchPacket(m_loginHandlers);
}

void CPythonNetworkStream::SetLoginPhase()
{
	if ("Login" != m_strPhase)
		m_phaseLeaveFunc.Run();

	Tracef("[PHASE] Entering phase: Login\n");
	Tracen("");
	Tracen("## Network - Login Phase ##");
	Tracen("");

	m_strPhase = "Login";
	RefreshProtocolHandlers();

	m_phaseProcessFunc.Set(this, &CPythonNetworkStream::LoginPhase);
	m_phaseLeaveFunc.Set(this, &CPythonNetworkStream::__LeaveLoginPhase);

	m_dwChangingPhaseTime = ELTimer_GetMSec();

	if (0 == m_dwLoginKey)
	{
		ClearLoginInfo();
		return;
	}

	if (__DirectEnterMode_IsSet())
	{
		SendLoginPacketNew(m_stID.c_str(), m_stPassword.c_str());
	}
	else
	{
		SendLoginPacketNew(m_stID.c_str(), m_stPassword.c_str());

		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOGIN], "OnLoginStart", Py_BuildValue("()"));

		__ClearSelectCharacterData();
	}
}

bool CPythonNetworkStream::__RecvEmpirePacket()
{
	TPacketGCEmpire kPacketEmpire;
	if (!Recv(sizeof(kPacketEmpire), &kPacketEmpire))
		return false;

	m_dwEmpireID=kPacketEmpire.bEmpire;
	return true;
}

bool CPythonNetworkStream::__RecvLoginSuccessPacket3()
{
	TPacketGCLoginSuccess3 kPacketLoginSuccess;

	if (!Recv(sizeof(kPacketLoginSuccess), &kPacketLoginSuccess))
		return false;

	for (int i = 0; i<PLAYER_PER_ACCOUNT3; ++i)
	{
		m_akSimplePlayerInfo[i]=kPacketLoginSuccess.akSimplePlayerInformation[i];
		m_adwGuildID[i]=kPacketLoginSuccess.guild_id[i];
		m_astrGuildName[i]=kPacketLoginSuccess.guild_name[i];
	}

	m_kMarkAuth.m_dwHandle=kPacketLoginSuccess.handle;
	m_kMarkAuth.m_dwRandomKey=kPacketLoginSuccess.random_key;	

	if (__DirectEnterMode_IsSet())
	{
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "Refresh", Py_BuildValue("()"));		
	}

	return true;
}

bool CPythonNetworkStream::__RecvLoginSuccessPacket4()
{
	TPacketGCLoginSuccess4 kPacketLoginSuccess;

	if (!Recv(sizeof(kPacketLoginSuccess), &kPacketLoginSuccess))
		return false;

	for (int i = 0; i<PLAYER_PER_ACCOUNT4; ++i)
	{
		m_akSimplePlayerInfo[i]=kPacketLoginSuccess.akSimplePlayerInformation[i];
		m_adwGuildID[i]=kPacketLoginSuccess.guild_id[i];
		m_astrGuildName[i]=kPacketLoginSuccess.guild_name[i];
	}

	m_kMarkAuth.m_dwHandle=kPacketLoginSuccess.handle;
	m_kMarkAuth.m_dwRandomKey=kPacketLoginSuccess.random_key;

	if (!__DirectEnterMode_IsSet())
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "Refresh", Py_BuildValue("()"));
	}

	return true;
}

void CPythonNetworkStream::OnConnectFailure()
{
	UserInterface::FastBoot::FastBootOverlay::Instance().SetLoginFailure("CONNECT_FAIL");

	if (__DirectEnterMode_IsSet())
	{
		ClosePhase();
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOGIN], "OnConnectFailure", Py_BuildValue("()"));	
	}
}

bool CPythonNetworkStream::__RecvLoginFailurePacket()
{
	TPacketGCLoginFailure packet_failure;
	if (!Recv(sizeof(TPacketGCLoginFailure), &packet_failure))
		return false;

	UserInterface::FastBoot::FastBootOverlay::Instance().SetLoginFailure(packet_failure.szStatus);

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOGIN], "OnLoginFailure", Py_BuildValue("(s)", packet_failure.szStatus));
#ifdef _DEBUG
	Tracef(" RecvLoginFailurePacket : [%s]\n", packet_failure.szStatus);
#endif
	return true;
}

bool CPythonNetworkStream::SendLoginPacketNew(const char * c_szName, const char * /*c_szPassword*/)
{
	auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	if (!pDriver)
	{
		Network::Protocol::ProtocolDriverRegistry::Instance().InitializeDefaults();
		pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	}

	auto loginRes = pDriver->EncodeLogin(c_szName ? c_szName : "", m_dwLoginKey);
	if (!loginRes.has_value())
	{
		Tracen("SendLoginPacketNew: Driver EncodeLogin failed");
		return false;
	}

	const auto& loginData = loginRes.value();
	if (!Send(static_cast<int>(loginData.size()), loginData.data()))
	{
		Tracen("SendLogin Error");
		return false;
	}

	__SendInternalBuffer();
	return true;
}

bool CPythonNetworkStream::RecvLoginSuccessPacketLuria()
{
	// Pakiet 0x20 o rozmiarze 489 bajtow:
	// header: 1B (0x20)
	// 5 slotow postaci po 79 bajtow (395B)
	// pozostale dane (93B): guild id, guild name, handle, random_key
	uint8_t buffer[489]{};
	if (!Recv(sizeof(buffer), buffer))
		return false;

	for (int s = 0; s < 5 && s < PLAYER_PER_ACCOUNT4; ++s)
	{
		int offset = 1 + s * 79;
		uint32_t charId = 0;
		std::memcpy(&charId, &buffer[offset], sizeof(uint32_t));

		if (charId != 0)
		{
			m_akSimplePlayerInfo[s].dwID = charId;
			std::memcpy(m_akSimplePlayerInfo[s].szName, &buffer[offset + 4], 24);
			m_akSimplePlayerInfo[s].szName[24] = '\0';
			m_akSimplePlayerInfo[s].byJob = buffer[offset + 29];
			m_akSimplePlayerInfo[s].byLevel = buffer[offset + 30];
			std::memcpy(&m_akSimplePlayerInfo[s].dwPlayMinutes, &buffer[offset + 31], sizeof(uint32_t));
			m_akSimplePlayerInfo[s].byST = buffer[offset + 35];
			m_akSimplePlayerInfo[s].byHT = buffer[offset + 36];
			m_akSimplePlayerInfo[s].byDX = buffer[offset + 37];
			m_akSimplePlayerInfo[s].byIQ = buffer[offset + 38];
			std::memcpy(&m_akSimplePlayerInfo[s].wMainPart, &buffer[offset + 39], sizeof(uint16_t));
			m_akSimplePlayerInfo[s].bChangeName = buffer[offset + 41];
			std::memcpy(&m_akSimplePlayerInfo[s].wHairPart, &buffer[offset + 42], sizeof(uint16_t));
			m_akSimplePlayerInfo[s].bySkillGroup = buffer[offset + 48];
			std::memcpy(&m_akSimplePlayerInfo[s].x, &buffer[offset + 64], sizeof(int32_t));
			std::memcpy(&m_akSimplePlayerInfo[s].y, &buffer[offset + 68], sizeof(int32_t));
			std::memcpy(&m_akSimplePlayerInfo[s].lAddr, &buffer[offset + 72], sizeof(uint32_t));
			std::memcpy(&m_akSimplePlayerInfo[s].wPort, &buffer[offset + 76], sizeof(uint16_t));
		}
		else
		{
			std::memset(&m_akSimplePlayerInfo[s], 0, sizeof(TSimplePlayerInformation));
		}
	}

	if (__DirectEnterMode_IsSet())
	{
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "Refresh", Py_BuildValue("()"));
	}

	return true;
}

bool CPythonNetworkStream::RecvLoginFailurePacketLuria()
{
	uint8_t header = 0;
	char szStatus[9]{};
	if (!Recv(sizeof(header), &header) || !Recv(8, szStatus))
		return false;

	uint8_t dummy = 0;
	Recv(sizeof(dummy), &dummy); // dopelnienie 10B

	UserInterface::FastBoot::FastBootOverlay::Instance().SetLoginFailure(szStatus);

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOGIN], "OnLoginFailure", Py_BuildValue("(s)", szStatus));
	return true;
}

bool CPythonNetworkStream::RecvEmpirePacketLuria()
{
	uint8_t header = 0;
	uint8_t empire = 0;
	if (!Recv(sizeof(header), &header) || !Recv(sizeof(empire), &empire))
		return false;

	m_dwEmpireID = empire;
	return true;
}

bool CPythonNetworkStream::__RecvLoginKeyPacket()
{
	TPacketGCLoginKey kLoginKeyPacket;
	if (!Recv(sizeof(TPacketGCLoginKey), &kLoginKeyPacket))
		return false;

	m_dwLoginKey = kLoginKeyPacket.dwLoginKey;
	m_isWaitLoginKey = FALSE;

	return true;
}
