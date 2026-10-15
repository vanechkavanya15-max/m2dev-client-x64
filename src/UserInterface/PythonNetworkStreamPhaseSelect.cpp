#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "Packet.h"
#include "Client/Network/Domain/SelectCommands.h"
#include "Client/Network/Protocol/ProtocolDriverRegistry.h"

// Select Character ---------------------------------------------------------------------------
void CPythonNetworkStream::SetSelectPhase()
{
	if ("Select" != m_strPhase)
		m_phaseLeaveFunc.Run();

	m_strPhase = "Select";
	m_bEnterGameSent = false;
	RefreshProtocolHandlers();

	m_dwChangingPhaseTime = ELTimer_GetMSec();
	m_phaseProcessFunc.Set(this, &CPythonNetworkStream::SelectPhase);
	m_phaseLeaveFunc.Set(this, &CPythonNetworkStream::__LeaveSelectPhase);

	extern bool g_bFastBootAutoConnect;
	if (g_bFastBootAutoConnect || __DirectEnterMode_IsSet())
	{
		BYTE slotIndex = __DirectEnterMode_IsSet() ? static_cast<BYTE>(m_kDirectEnterMode.m_dwChrSlotIndex) : static_cast<BYTE>(m_dwSelectedCharacterIndex);
		m_dwSelectedCharacterIndex = slotIndex;

		if (!IsCharacterSlotEmpty(slotIndex))
		{
			__DirectEnterMode_Set(slotIndex);
			TraceError("[FAST_BOOT] SetSelectPhase: Natychmiastowe wyslanie SendSelectCharacterPacket(slot=%u) bez animacji introSelect", slotIndex);
			SendSelectCharacterPacket(slotIndex);

			if (m_poHandler)
			{
				PyCallClassMemberFunc(m_poHandler, "SetLoadingPhase", Py_BuildValue("()"));
			}
		}
		else
		{
			TraceError("[FAST_BOOT] SetSelectPhase: Slot=%u jest PUSTY! Oczekiwanie na stworzenie postaci.", slotIndex);
			if (m_poHandler)
			{
				PyCallClassMemberFunc(m_poHandler, "SetSelectCharacterPhase", Py_BuildValue("()"));
			}
		}
	}
	else
	{
		if (IsSelectedEmpire())
			PyCallClassMemberFunc(m_poHandler, "SetSelectCharacterPhase", Py_BuildValue("()"));
		else
			PyCallClassMemberFunc(m_poHandler, "SetSelectEmpirePhase", Py_BuildValue("()"));
	}
}

void CPythonNetworkStream::SelectPhase()
{
	while (m_strPhase == "Select" && !m_isStartGame && DispatchPacket(m_selectHandlers))
		;
}

bool CPythonNetworkStream::SendSelectEmpirePacket(DWORD dwEmpireID)
{
	auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	if (pDriver)
	{
		auto encodedResult = pDriver->EncodeSelectEmpire(Domain::EmpireSelectCommand{ (uint8_t)dwEmpireID });
		if (encodedResult.has_value() && !encodedResult.value().empty())
		{
			if (!Send(static_cast<int>(encodedResult.value().size()), encodedResult.value().data()))
			{
				Tracen("SendSelectEmpirePacket Driver - Error");
				return false;
			}
			__SendInternalBuffer();
			SetEmpireID(dwEmpireID);
			return true;
		}
	}

	TPacketCGEmpire kPacketEmpire;
	kPacketEmpire.header=CG::EMPIRE;
	kPacketEmpire.length = sizeof(kPacketEmpire);
	kPacketEmpire.bEmpire=dwEmpireID;

	if (!Send(sizeof(kPacketEmpire), &kPacketEmpire))
	{
		Tracen("SendSelectEmpirePacket - Error");
		return false;
	}

	__SendInternalBuffer();
	SetEmpireID(dwEmpireID);
	return true;
}

bool CPythonNetworkStream::SendSelectCharacterPacket(BYTE Index)
{
	m_dwSelectedCharacterIndex = Index;
	TraceError("[SELECT_DEBUG] SendSelectCharacterPacket called for slot=%d", Index);

	extern bool g_bFastBootAutoConnect;
	if (g_bFastBootAutoConnect)
	{
		__DirectEnterMode_Set(Index);
	}

	auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	if (pDriver)
	{
		auto encodedResult = pDriver->EncodeSelectCharacter(Domain::CharacterSelectCommand{ Index });
		if (encodedResult.has_value() && !encodedResult.value().empty())
		{
			if (!Send(static_cast<int>(encodedResult.value().size()), encodedResult.value().data()))
			{
				Tracen("SendSelectCharacterPacket Driver - Error");
				return false;
			}
			__SendInternalBuffer();
			TraceError("[SELECT_DEBUG] SendSelectCharacterPacket driver encoded %zu bytes sent and flushed", encodedResult.value().size());
			return true;
		}
	}

	TPacketCGSelectCharacter SelectCharacterPacket;

	SelectCharacterPacket.header = CG::CHARACTER_SELECT;
	SelectCharacterPacket.length = sizeof(SelectCharacterPacket);
	SelectCharacterPacket.player_index = Index;

	if (!Send(sizeof(TPacketCGSelectCharacter), &SelectCharacterPacket))
	{
		Tracen("SendSelectCharacterPacket - Error");
		return false;
	}

	__SendInternalBuffer();
	TraceError("[SELECT_DEBUG] SendSelectCharacterPacket fallback sent and flushed");
	return true;
}

bool CPythonNetworkStream::SendDestroyCharacterPacket(BYTE index, const char * szPrivateCode)
{
	auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	if (pDriver)
	{
		auto encodedResult = pDriver->EncodeDestroyCharacter(Domain::CharacterDestroyCommand{ index, szPrivateCode ? szPrivateCode : "" });
		if (encodedResult.has_value() && !encodedResult.value().empty())
		{
			if (!Send(static_cast<int>(encodedResult.value().size()), encodedResult.value().data()))
			{
				Tracen("SendDestroyCharacterPacket Driver - Error");
				return false;
			}
			return true;
		}
	}

    TPacketCGDestroyCharacter DestroyCharacterPacket;

	DestroyCharacterPacket.header = CG::CHARACTER_DELETE;
	DestroyCharacterPacket.length = sizeof(DestroyCharacterPacket);
	DestroyCharacterPacket.index = index;
	strncpy(DestroyCharacterPacket.szPrivateCode, szPrivateCode, PRIVATE_CODE_LENGTH-1);

	if (!Send(sizeof(TPacketCGDestroyCharacter), &DestroyCharacterPacket))
	{
		Tracen("SendDestroyCharacterPacket");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendCreateCharacterPacket(BYTE index, const char *name, BYTE job, BYTE shape, BYTE byCON, BYTE byINT, BYTE bySTR, BYTE byDEX)
{
	auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	if (pDriver)
	{
		auto encodedResult = pDriver->EncodeCreateCharacter(Domain::CharacterCreateCommand{ index, name ? name : "", (uint16_t)job, shape, byCON, byINT, bySTR, byDEX });
		if (encodedResult.has_value() && !encodedResult.value().empty())
		{
			if (!Send(static_cast<int>(encodedResult.value().size()), encodedResult.value().data()))
			{
				Tracen("Failed to SendCreateCharacterPacket Driver");
				return false;
			}
			return true;
		}
	}

	TPacketCGCreateCharacter createCharacterPacket;

	createCharacterPacket.header = CG::CHARACTER_CREATE;
	createCharacterPacket.length = sizeof(createCharacterPacket);
	createCharacterPacket.index = index;
	strncpy(createCharacterPacket.name, name, CHARACTER_NAME_MAX_LEN);
	createCharacterPacket.job = job;
	createCharacterPacket.shape = shape;
	createCharacterPacket.CON = byCON;
	createCharacterPacket.INT = byINT;
	createCharacterPacket.STR = bySTR;
	createCharacterPacket.DEX = byDEX;

	if (!Send(sizeof(TPacketCGCreateCharacter), &createCharacterPacket))
	{
		Tracen("Failed to SendCreateCharacterPacket");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendChangeNamePacket(BYTE index, const char *name)
{
	TPacketCGChangeName ChangeNamePacket;
	ChangeNamePacket.header = CG::CHANGE_NAME;
	ChangeNamePacket.length = sizeof(ChangeNamePacket);
	ChangeNamePacket.index = index;
	strncpy(ChangeNamePacket.name, name, CHARACTER_NAME_MAX_LEN);

	if (!Send(sizeof(TPacketCGChangeName), &ChangeNamePacket))
	{
		Tracen("Failed to SendChangeNamePacket");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::__RecvPlayerCreateSuccessPacket()
{
	TPacketGCPlayerCreateSuccess kCreateSuccessPacket;

	if (!Recv(sizeof(kCreateSuccessPacket), &kCreateSuccessPacket))
		return false;

	if (kCreateSuccessPacket.bAccountCharacterSlot>=PLAYER_PER_ACCOUNT4)
	{
		TraceError("CPythonNetworkStream::RecvPlayerCreateSuccessPacket - OUT OF RANGE SLOT(%d) > PLATER_PER_ACCOUNT(%d)",
			kCreateSuccessPacket.bAccountCharacterSlot, PLAYER_PER_ACCOUNT4);
		return true;
	}

	m_akSimplePlayerInfo[kCreateSuccessPacket.bAccountCharacterSlot]=kCreateSuccessPacket.kSimplePlayerInfomation;
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_CREATE], "OnCreateSuccess", Py_BuildValue("()"));

	extern bool g_bFastBootAutoConnect;
	if (g_bFastBootAutoConnect || __DirectEnterMode_IsSet())
	{
		BYTE slotIndex = kCreateSuccessPacket.bAccountCharacterSlot;
		TraceError("[FAST_BOOT] Postac stworzona pomyslnie na slocie %u! Natychmiastowe wejscie do gry...", slotIndex);
		__DirectEnterMode_Set(slotIndex);
		SendSelectCharacterPacket(slotIndex);
		if (m_poHandler)
		{
			PyCallClassMemberFunc(m_poHandler, "SetLoadingPhase", Py_BuildValue("()"));
		}
	}
	return true;
}

bool CPythonNetworkStream::__RecvPlayerCreateFailurePacket()
{
	TPacketGCCreateFailure packet;

	if (!Recv(sizeof(TPacketGCCreateFailure), &packet))
		return false;

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_CREATE], "OnCreateFailure", Py_BuildValue("(i)", packet.bType));
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnCreateFailure", Py_BuildValue("(i)", packet.bType));
	return true;
}

bool CPythonNetworkStream::__RecvPlayerDestroySuccessPacket()
{
	TPacketGCDestroyCharacterSuccess packet;
	if (!Recv(sizeof(TPacketGCDestroyCharacterSuccess), &packet))
		return false;

	memset(&m_akSimplePlayerInfo[packet.account_index], 0, sizeof(m_akSimplePlayerInfo[packet.account_index]));
	m_adwGuildID[packet.account_index] = 0;
	m_astrGuildName[packet.account_index] = "";

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnDeleteSuccess", Py_BuildValue("(i)", packet.account_index));
	return true;
}

bool CPythonNetworkStream::__RecvPlayerDestroyFailurePacket()
{
	TPacketGCBlank packet_blank;
	if (!Recv(sizeof(TPacketGCBlank), &packet_blank))
		return false;

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnDeleteFailure", Py_BuildValue("()"));
	return true;
}

bool CPythonNetworkStream::__RecvChangeName()
{
	TPacketGCChangeName ChangeNamePacket;
	if (!Recv(sizeof(TPacketGCChangeName), &ChangeNamePacket))
		return false;

	for (int i = 0; i < PLAYER_PER_ACCOUNT4; ++i)
	{
		if (ChangeNamePacket.pid == m_akSimplePlayerInfo[i].dwID)
		{
			m_akSimplePlayerInfo[i].bChangeName = FALSE;
			strncpy(m_akSimplePlayerInfo[i].szName, ChangeNamePacket.name, CHARACTER_NAME_MAX_LEN);

			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnChangeName", Py_BuildValue("(is)", i, ChangeNamePacket.name));
			return true;
		}
	}

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnCreateFailure", Py_BuildValue("(i)", 100));
	return true;
}

bool CPythonNetworkStream::RecvPlayerCreateSuccessPacketLuria()
{
	uint8_t header = 0;
	uint8_t slot = 0;
	if (!Recv(sizeof(header), &header) || !Recv(sizeof(slot), &slot))
		return false;

	if (slot >= PLAYER_PER_ACCOUNT4)
	{
		TraceError("CPythonNetworkStream::RecvPlayerCreateSuccessPacketLuria - OUT OF RANGE SLOT(%d)", slot);
		return true;
	}

	uint8_t charBuf[79]{};
	if (!Recv(sizeof(charBuf), charBuf))
		return false;

	uint32_t charId = 0;
	std::memcpy(&charId, &charBuf[0], sizeof(uint32_t));
	m_akSimplePlayerInfo[slot].dwID = charId;
	std::memcpy(m_akSimplePlayerInfo[slot].szName, &charBuf[4], 24);
	m_akSimplePlayerInfo[slot].szName[24] = '\0';
	m_akSimplePlayerInfo[slot].byJob = charBuf[29];
	m_akSimplePlayerInfo[slot].byLevel = charBuf[30];
	std::memcpy(&m_akSimplePlayerInfo[slot].dwPlayMinutes, &charBuf[31], sizeof(uint32_t));
	m_akSimplePlayerInfo[slot].byST = charBuf[35];
	m_akSimplePlayerInfo[slot].byHT = charBuf[36];
	m_akSimplePlayerInfo[slot].byDX = charBuf[37];
	m_akSimplePlayerInfo[slot].byIQ = charBuf[38];
	std::memcpy(&m_akSimplePlayerInfo[slot].wMainPart, &charBuf[39], sizeof(uint16_t));
	m_akSimplePlayerInfo[slot].bChangeName = charBuf[41];
	std::memcpy(&m_akSimplePlayerInfo[slot].wHairPart, &charBuf[42], sizeof(uint16_t));
	m_akSimplePlayerInfo[slot].bySkillGroup = charBuf[48];
	std::memcpy(&m_akSimplePlayerInfo[slot].x, &charBuf[64], sizeof(int32_t));
	std::memcpy(&m_akSimplePlayerInfo[slot].y, &charBuf[68], sizeof(int32_t));
	std::memcpy(&m_akSimplePlayerInfo[slot].lAddr, &charBuf[72], sizeof(uint32_t));
	std::memcpy(&m_akSimplePlayerInfo[slot].wPort, &charBuf[76], sizeof(uint16_t));

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_CREATE], "OnCreateSuccess", Py_BuildValue("()"));
	return true;
}

bool CPythonNetworkStream::RecvPlayerCreateFailurePacketLuria()
{
	uint8_t header = 0;
	uint8_t bType = 0;
	if (!Recv(sizeof(header), &header) || !Recv(sizeof(bType), &bType))
		return false;

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_CREATE], "OnCreateFailure", Py_BuildValue("(i)", bType));
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnCreateFailure", Py_BuildValue("(i)", bType));
	return true;
}

bool CPythonNetworkStream::RecvPlayerDestroySuccessPacketLuria()
{
	uint8_t header = 0;
	uint8_t slot = 0;
	if (!Recv(sizeof(header), &header) || !Recv(sizeof(slot), &slot))
		return false;

	if (slot < PLAYER_PER_ACCOUNT4)
	{
		std::memset(&m_akSimplePlayerInfo[slot], 0, sizeof(m_akSimplePlayerInfo[slot]));
		m_adwGuildID[slot] = 0;
		m_astrGuildName[slot] = "";
	}

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnDeleteSuccess", Py_BuildValue("(i)", slot));
	return true;
}

bool CPythonNetworkStream::RecvPlayerDestroyFailurePacketLuria()
{
	uint8_t header = 0;
	if (!Recv(sizeof(header), &header))
		return false;

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnDeleteFailure", Py_BuildValue("()"));
	return true;
}
