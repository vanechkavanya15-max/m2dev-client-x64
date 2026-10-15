#include "StdAfx.h"
#include <span>
#include "PythonNetworkStream.h"
#include "Packet.h"
#include "Network/Dispatchers/NetworkStreamPhaseGameBridge.h"
#include "GuildMarkDownloader.h"
#include "MarkManager.h"
#include "Client/Network/Protocol/ProtocolDriverRegistry.h"
#include "Client/Network/Protocol/LuriaProtocol.h"
#include "Client/Network/Domain/MovementCommands.h"
#include "Client/Network/Domain/CombatCommands.h"

#include "PythonGuild.h"
#include "PythonCharacterManager.h"
#include "PythonPlayer.h"
#include "PythonBackground.h"
#include "PythonMiniMap.h"
#include "PythonTextTail.h"
#include "PythonItem.h"
#include "PythonChat.h"
#include "PythonShop.h"
#include "PythonExchange.h"
#include "PythonQuest.h"
#include "PythonEventManager.h"
#include "PythonMessenger.h"
#include "PythonApplication.h"

#include "GameLib/ItemManager.h"

#include "AbstractApplication.h"
#include "AbstractCharacterManager.h"
#include "InstanceBase.h"

#include "ProcessCRC.h"
#include "Client/Bridge/StranglerFacade.h"
#include "PythonNetworkStreamPhaseGameGuild.h"
#include "PythonNetworkStreamPhaseGameParty.h"
#include "PythonNetworkStreamPhaseGameQuest.h"
#include "PythonNetworkStreamPhaseGameSkills.h"
#include "PythonNetworkStreamPhaseGameTarget.h"
#include "PythonNetworkStreamPhaseGameShop.h"
#include "PythonNetworkStreamPhaseGameExchange.h"
#include "PythonNetworkStreamPhaseGameCombat.h"
#include "PythonNetworkStreamPhaseGameWorld.h"
#include "PythonNetworkStreamPhaseGameRefine.h"
#include "PythonNetworkStreamPhaseGameChat.h"
#include "PythonNetworkStreamPhaseGameSync.h"
#include "Client/Network/Handlers/PartyPacketDomainHandler.h"
#include "Client/Network/Handlers/GuildPacketDomainHandler.h"
#include "Client/Network/Handlers/QuestDialogDomainHandler.h"
#include "Client/Network/Handlers/RefineExchangeDomainHandler.h"

BOOL gs_bEmpireLanuageEnable = TRUE;

void CPythonNetworkStream::__RefreshAlignmentWindow()
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshAlignment", Py_BuildValue("()"));
}

void CPythonNetworkStream::__RefreshTargetBoardByVID(DWORD dwVID)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshTargetBoardByVID", Py_BuildValue("(i)", dwVID));
}

void CPythonNetworkStream::__RefreshTargetBoardByName(const char * c_szName)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshTargetBoardByName", Py_BuildValue("(s)", c_szName));
}

void CPythonNetworkStream::__RefreshTargetBoard()
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshTargetBoard", Py_BuildValue("()"));
}

void CPythonNetworkStream::__RefreshGuildWindowGradePage()
{
	m_isRefreshGuildWndGradePage=true;
}

void CPythonNetworkStream::__RefreshGuildWindowSkillPage()
{
	m_isRefreshGuildWndSkillPage=true;
}

void CPythonNetworkStream::__RefreshGuildWindowMemberPageGradeComboBox()
{
	m_isRefreshGuildWndMemberPageGradeComboBox=true;
}

void CPythonNetworkStream::__RefreshGuildWindowMemberPage()
{
	m_isRefreshGuildWndMemberPage=true;
}

void CPythonNetworkStream::__RefreshGuildWindowBoardPage()
{
	m_isRefreshGuildWndBoardPage=true;
}

void CPythonNetworkStream::__RefreshGuildWindowInfoPage()
{
	m_isRefreshGuildWndInfoPage=true;
}

void CPythonNetworkStream::__RefreshMessengerWindow()
{
	m_isRefreshMessengerWnd=true;
}

void CPythonNetworkStream::__RefreshSafeboxWindow()
{
	m_isRefreshSafeboxWnd=true;
}

void CPythonNetworkStream::__RefreshMallWindow()
{
	m_isRefreshMallWnd=true;
}

void CPythonNetworkStream::__RefreshSkillWindow()
{
	m_isRefreshSkillWnd=true;
}

void CPythonNetworkStream::__RefreshExchangeWindow()
{
	m_isRefreshExchangeWnd=true;
}

void CPythonNetworkStream::__RefreshStatus()
{
	m_isRefreshStatus=true;
}

void CPythonNetworkStream::__RefreshCharacterWindow()
{
	m_isRefreshCharacterWnd=true;
}

void CPythonNetworkStream::__RefreshInventoryWindow()
{
	m_isRefreshInventoryWnd=true;
}

void CPythonNetworkStream::__RefreshEquipmentWindow()
{
	m_isRefreshEquipmentWnd=true;
}

void CPythonNetworkStream::__SetGuildID(DWORD id)
{
	if (m_dwGuildID != id)
	{
		m_dwGuildID = id;
		IAbstractPlayer& rkPlayer = IAbstractPlayer::GetSingleton();

		for (int i = 0; i < PLAYER_PER_ACCOUNT4; ++i)
			if (!strncmp(m_akSimplePlayerInfo[i].szName, rkPlayer.GetName(), CHARACTER_NAME_MAX_LEN))
			{
				m_adwGuildID[i] = id;

				std::string  guildName;
				if (CPythonGuild::Instance().GetGuildName(id, &guildName))
				{
					m_astrGuildName[i] = guildName;
				}
				else
				{
					m_astrGuildName[i] = "";
				}
			}
	}
}

struct PERF_PacketInfo
{
	DWORD dwCount;
	DWORD dwTime;

	PERF_PacketInfo()
	{
		dwCount=0;
		dwTime=0;
	}
};


#ifdef __PERFORMANCE_CHECK__

class PERF_PacketTimeAnalyzer
{
	public:
		~PERF_PacketTimeAnalyzer()
		{
			FILE* fp=fopen("perf_dispatch_packet_result.txt", "w");		

			for (std::map<DWORD, PERF_PacketInfo>::iterator i=m_kMap_kPacketInfo.begin(); i!=m_kMap_kPacketInfo.end(); ++i)
			{
				if (i->second.dwTime>0)
					fprintf(fp, "header %d: count %d, time %d, tpc %d\n", i->first, i->second.dwCount, i->second.dwTime, i->second.dwTime/i->second.dwCount);
			}
			fclose(fp);
		}

	public:
		std::map<DWORD, PERF_PacketInfo> m_kMap_kPacketInfo;
};

PERF_PacketTimeAnalyzer gs_kPacketTimeAnalyzer;

#endif

// Game Phase ---------------------------------------------------------------------------
void CPythonNetworkStream::GamePhase()
{
	if (!m_kQue_stHack.empty())
	{
		__SendHack(m_kQue_stHack.front().c_str());
		m_kQue_stHack.pop_front();
	}

#ifdef __PERFORMANCE_CHECK__
	DWORD timeBeginDispatch=timeGetTime();
#endif

	const DWORD MAX_RECV_COUNT = 32;
	const DWORD SAFE_RECV_BUFSIZE = 8192;
	DWORD dwRecvCount = 0;

	while (true)
	{
		if (dwRecvCount++ >= MAX_RECV_COUNT-1 && GetRecvBufferSize() < SAFE_RECV_BUFSIZE
			&& m_strPhase == "Game")
			break;

		if (!DispatchPacket(m_gameHandlers))
			break;
	}

#ifdef __PERFORMANCE_CHECK__
	DWORD timeEndDispatch=timeGetTime();

	if (timeEndDispatch-timeBeginDispatch>2)
	{
		static FILE* fp=fopen("perf_dispatch_packet.txt", "w");
		fprintf(fp, "delay %d\n", timeEndDispatch-timeBeginDispatch);
		fputs("=====================================================\n", fp);
		fflush(fp);
	}
#endif

	static DWORD s_nextRefreshTime = ELTimer_GetMSec();

	DWORD curTime = ELTimer_GetMSec();
	if (s_nextRefreshTime > curTime)
		return;	
	
	

	if (m_isRefreshCharacterWnd)
	{
		m_isRefreshCharacterWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshCharacter", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshEquipmentWnd)
	{
		m_isRefreshEquipmentWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshEquipment", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshInventoryWnd)
	{
		m_isRefreshInventoryWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshInventory", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshExchangeWnd)
	{
		m_isRefreshExchangeWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshExchange", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshSkillWnd)
	{
		m_isRefreshSkillWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshSkill", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshSafeboxWnd)
	{
		m_isRefreshSafeboxWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshSafebox", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshMallWnd)
	{
		m_isRefreshMallWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshMall", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshStatus)
	{
		m_isRefreshStatus=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshStatus", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshMessengerWnd)
	{
		m_isRefreshMessengerWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshMessenger", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshGuildWndInfoPage)
	{
		m_isRefreshGuildWndInfoPage=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildInfoPage", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshGuildWndBoardPage)
	{
		m_isRefreshGuildWndBoardPage=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildBoardPage", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshGuildWndMemberPage)
	{
		m_isRefreshGuildWndMemberPage=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildMemberPage", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshGuildWndMemberPageGradeComboBox)
	{
		m_isRefreshGuildWndMemberPageGradeComboBox=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildMemberPageGradeComboBox", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshGuildWndSkillPage)
	{
		m_isRefreshGuildWndSkillPage=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildSkillPage", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshGuildWndGradePage)
	{
		m_isRefreshGuildWndGradePage=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildGradePage", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}	
}

void CPythonNetworkStream::__InitializeGamePhase()
{
	__ServerTimeSync_Initialize();

	m_isRefreshStatus=false;
	m_isRefreshCharacterWnd=false;
	m_isRefreshEquipmentWnd=false;
	m_isRefreshInventoryWnd=false;
	m_isRefreshExchangeWnd=false;
	m_isRefreshSkillWnd=false;
	m_isRefreshSafeboxWnd=false;
	m_isRefreshMallWnd=false;
	m_isRefreshMessengerWnd=false;
	m_isRefreshGuildWndInfoPage=false;
	m_isRefreshGuildWndBoardPage=false;
	m_isRefreshGuildWndMemberPage=false;
	m_isRefreshGuildWndMemberPageGradeComboBox=false;
	m_isRefreshGuildWndSkillPage=false;
	m_isRefreshGuildWndGradePage=false;

	m_EmoticonStringVector.clear();

	m_pInstTarget = NULL;
}

void CPythonNetworkStream::Warp(LONG lGlobalX, LONG lGlobalY)
{
	PhaseGameSyncBridge::Warp(this, lGlobalX, lGlobalY);
}

void CPythonNetworkStream::__ShowMapName(LONG lLocalX, LONG lLocalY)
{
	PhaseGameSyncBridge::ShowMapName(this, lLocalX, lLocalY);
}

void CPythonNetworkStream::__LeaveGamePhase()
{
	PhaseGameSyncBridge::LeaveGamePhase(this);
}

void CPythonNetworkStream::SetGamePhase()
{
	PhaseGameSyncBridge::SetGamePhase(this);
}


// Observer i Warp - Zaimplementowane w PythonNetworkStreamPhaseGameMisc.cpp


bool CPythonNetworkStream::RecvDuelStartPacket()
{
	TPacketGCDuelStart kDuelStartPacket;
	if (!Recv(sizeof(kDuelStartPacket), &kDuelStartPacket))
		return false;
	
	DWORD count = (kDuelStartPacket.length - sizeof(kDuelStartPacket))/sizeof(DWORD);

	CPythonCharacterManager & rkChrMgr = CPythonCharacterManager::Instance();

	CInstanceBase* pkInstMain=rkChrMgr.GetMainActorPtr();
	if (!pkInstMain)
	{
		TraceError("CPythonNetworkStream::RecvDuelStartPacket - MainCharacter is NULL");
		return false;
	}
	DWORD dwVIDSrc = pkInstMain->GetVirtualID();
	DWORD dwVIDDest;

	for ( DWORD i = 0; i < count; i++)
	{
		Recv(sizeof(dwVIDDest),&dwVIDDest);
		CInstanceBase::InsertDUELKey(dwVIDSrc,dwVIDDest);
	}
	
	if(count == 0)
		pkInstMain->SetDuelMode(CInstanceBase::DUEL_CANNOTATTACK);
	else
		pkInstMain->SetDuelMode(CInstanceBase::DUEL_START);
	
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "CloseTargetBoard", Py_BuildValue("()"));
	
	rkChrMgr.RefreshAllPCTextTail();

	return true;
}

// RecvPVPPacket moved to Network/Handlers/PvpHandler.cpp

// DELETEME
/*
void CPythonNetworkStream::__SendWarpPacket()
{
	TPacketCGWarp kWarpPacket;
	kWarpPacket.header=GC::WARP;
	kWarpPacket.length = sizeof(kWarpPacket);
	if (!Send(sizeof(kWarpPacket), &kWarpPacket))
	{
		return;
	}
}
*/
void CPythonNetworkStream::NotifyHack(const char* c_szMsg)
{
	if (!m_kQue_stHack.empty())
		if (c_szMsg==m_kQue_stHack.back())
			return;

	m_kQue_stHack.push_back(c_szMsg);	
}

bool CPythonNetworkStream::__SendHack(const char* c_szMsg)
{
	Tracen(c_szMsg);
	
	TPacketCGHack kPacketHack;
	kPacketHack.header=CG::HACK;
	kPacketHack.length = sizeof(kPacketHack);
	strncpy(kPacketHack.szBuf, c_szMsg, sizeof(kPacketHack.szBuf)-1);

	if (!Send(sizeof(kPacketHack), &kPacketHack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendMessengerAddByVIDPacket(DWORD vid)
{
	TPacketCGMessenger packet;
	packet.header = CG::MESSENGER;
	packet.length = sizeof(packet) + sizeof(vid);
	packet.subheader = MessengerSub::CG::ADD_BY_VID;
	if (!Send(sizeof(packet), &packet))
		return false;
	if (!Send(sizeof(vid), &vid))
		return false;
	return true;
}

bool CPythonNetworkStream::SendMessengerAddByNamePacket(const char * c_szName)
{
	TPacketCGMessenger packet;
	packet.header = CG::MESSENGER;
	packet.length = sizeof(packet) + sizeof(char[CHARACTER_NAME_MAX_LEN]);
	packet.subheader = MessengerSub::CG::ADD_BY_NAME;
	if (!Send(sizeof(packet), &packet))
		return false;
	char szName[CHARACTER_NAME_MAX_LEN];
	strncpy(szName, c_szName, CHARACTER_NAME_MAX_LEN-1);
	szName[CHARACTER_NAME_MAX_LEN-1] = '\0'; // #720: 메신저 이름 관련 버퍼 오버플로우 버그 수정

	if (!Send(sizeof(szName), &szName))
		return false;
	Tracef(" SendMessengerAddByNamePacket : %s\n", c_szName);
	return true;
}

bool CPythonNetworkStream::SendMessengerRemovePacket(const char * c_szKey, const char * c_szName)
{
	TPacketCGMessenger packet;
	packet.header = CG::MESSENGER;
	packet.length = sizeof(packet) + sizeof(char[CHARACTER_NAME_MAX_LEN]);
	packet.subheader = MessengerSub::CG::REMOVE;
	if (!Send(sizeof(packet), &packet))
		return false;
	char szKey[CHARACTER_NAME_MAX_LEN];
	strncpy(szKey, c_szKey, CHARACTER_NAME_MAX_LEN-1);
	if (!Send(sizeof(szKey), &szKey))
		return false;
	__RefreshTargetBoardByName(c_szName);
	return true;
}

bool CPythonNetworkStream::SendCharacterStatePacket(const TPixelPosition& c_rkPPosDst, float fDstRot, UINT eFunc, UINT uArg)
{
	if (!__CanActMainInstance())
		return true;

	// Strangler Fig C++23: Przekierowanie wykonania do nowoczesnej domeny i WorldContext
	(void)Client::Bridge::StranglerFacade::Instance().ExecuteMove(c_rkPPosDst.x, c_rkPPosDst.y, c_rkPPosDst.z, fDstRot, static_cast<uint8_t>(eFunc));

	if (fDstRot < 0.0f)
		fDstRot = 360 + fDstRot;
	else if (fDstRot > 360.0f)
		fDstRot = fmodf(fDstRot, 360.0f);

	// TODO: 나중에 패킷이름을 바꾸자
	TPacketCGMove kStatePacket;
	kStatePacket.header = CG::MOVE;
	kStatePacket.length = sizeof(kStatePacket);
	kStatePacket.bFunc = eFunc;
	kStatePacket.bArg = uArg;
	kStatePacket.bRot = fDstRot/5.0f;
	kStatePacket.lX = long(c_rkPPosDst.x);
	kStatePacket.lY = long(c_rkPPosDst.y);
	kStatePacket.dwTime = ELTimer_GetServerMSec();
	
	if (kStatePacket.lX < 0 || kStatePacket.lX >= 204800 || kStatePacket.lY < 0 || kStatePacket.lY >= 204800)
	{
		TraceError("SendCharacterStatePacket INVALID LOCAL COORDS: lX=%d, lY=%d (c_rkPPosDst=(%f, %f))",
			kStatePacket.lX, kStatePacket.lY, c_rkPPosDst.x, c_rkPPosDst.y);
	}

	__LocalPositionToGlobalPosition(kStatePacket.lX, kStatePacket.lY);

	auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	if (pDriver && pDriver->GetDriverName() != "standard_x64")
	{
		Network::Domain::MoveCommand cmd{
			.vid = 0,
			.x = kStatePacket.lX,
			.y = kStatePacket.lY,
			.rotationDegrees = fDstRot,
			.time = kStatePacket.dwTime,
			.func = static_cast<uint8_t>(eFunc),
			.arg = static_cast<uint8_t>(uArg)
		};
		auto enc = pDriver->EncodeMove(cmd);
		if (enc.has_value())
		{
			const auto& buf = enc.value();
			if (!Send(static_cast<int>(buf.size()), buf.data()))
			{
				Tracenf("CPythonNetworkStream::SendCharacterStatePacket [UPM: %s] SEND ERROR", pDriver->GetDriverName().data());
				return false;
			}
			return true;
		}
	}

	if (!Send(sizeof(kStatePacket), &kStatePacket))
	{
		Tracenf("CPythonNetworkStream::SendCharacterStatePacket(dwCmdTime=%u, fDstPos=(%f, %f), fDstRot=%f, eFunc=%d uArg=%d) - PACKET SEND ERROR",
			kStatePacket.dwTime,
			float(kStatePacket.lX),
			float(kStatePacket.lY),
			fDstRot,
			kStatePacket.bFunc,
			kStatePacket.bArg);
		return false;
	}
	return true;
}

// NOTE : SlotIndex는 임시
bool CPythonNetworkStream::SendUseSkillPacket(DWORD dwSkillIndex, DWORD dwTargetVID)
{
	TPacketCGUseSkill UseSkillPacket;

	UseSkillPacket.header = CG::USE_SKILL;
	UseSkillPacket.length = sizeof(UseSkillPacket);
	UseSkillPacket.dwVnum = dwSkillIndex;
	UseSkillPacket.dwTargetVID = dwTargetVID;

	// tw1x1 fix wrong fly targeting for viewing clients
	if (dwTargetVID)
	{
		CPythonCharacterManager& rpcm = CPythonCharacterManager::Instance();
		CInstanceBase* pTarget = rpcm.GetInstancePtr(dwTargetVID);

		if (pTarget)
		{
			TPixelPosition kPos;

			pTarget->NEW_GetPixelPosition(&kPos);
			SendFlyTargetingPacket(dwTargetVID, kPos);
		}
		else
		{
			TPixelPosition kPos;

			kPos.x = 0;
			kPos.y = 0;
			kPos.z = 0;

			SendFlyTargetingPacket(0, kPos);
		}
	}
	// END OF tw1x1 fix wrong fly targeting for viewing clients

	if (!Send(sizeof(TPacketCGUseSkill), &UseSkillPacket))
	{
		Tracen("CPythonNetworkStream::SendUseSkillPacket - SEND PACKET ERROR");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendChatPacket(const char * c_szChat, BYTE byType)
{
	return PhaseGameChatBridge::SendChat(this, c_szChat, byType);
}


//////////////////////////////////////////////////////////////////////////
// Emoticon
void CPythonNetworkStream::RegisterEmoticonString(const char * pcEmoticonString)
{
	if (m_EmoticonStringVector.size() >= CInstanceBase::EMOTICON_NUM)
	{
		TraceError("Can't register emoticon string... vector is full (size:%d)", m_EmoticonStringVector.size() );
		return;
	}
	m_EmoticonStringVector.push_back(pcEmoticonString);
}

bool CPythonNetworkStream::ParseEmoticon(const char * pChatMsg, DWORD * pdwEmoticon)
{
	for (DWORD dwEmoticonIndex = 0; dwEmoticonIndex < m_EmoticonStringVector.size() ; ++dwEmoticonIndex)
	{
		if (strlen(pChatMsg) > m_EmoticonStringVector[dwEmoticonIndex].size())
			continue;

		const char * pcFind = strstr(pChatMsg, m_EmoticonStringVector[dwEmoticonIndex].c_str());

		if (pcFind != pChatMsg)
			continue;

		*pdwEmoticon = dwEmoticonIndex;

		return true;
	}

	return false;
}
// Emoticon
//////////////////////////////////////////////////////////////////////////

void CPythonNetworkStream::__ConvertEmpireText(DWORD dwEmpireID, char* szText)
{
	if (dwEmpireID<1 || dwEmpireID>3)
		return;

	UINT uHanPos;

	STextConvertTable& rkTextConvTable=m_aTextConvTable[dwEmpireID-1];

	BYTE* pbText=(BYTE*)szText;
	while (*pbText)
	{
		if (*pbText & 0x80)
		{
			if (pbText[0]>=0xb0 && pbText[0]<=0xc8 && pbText[1]>=0xa1 && pbText[1]<=0xfe)
			{
				uHanPos=(pbText[0]-0xb0)*(0xfe-0xa1+1)+(pbText[1]-0xa1);
				pbText[0]=rkTextConvTable.aacHan[uHanPos][0];
				pbText[1]=rkTextConvTable.aacHan[uHanPos][1];
			}
			pbText+=2;
		}
		else
		{
			if (*pbText>='a' && *pbText<='z')
			{
				*pbText=rkTextConvTable.acLower[*pbText-'a'];
			}
			else if (*pbText>='A' && *pbText<='Z')
			{
				*pbText=rkTextConvTable.acUpper[*pbText-'A'];
			}
			pbText++;
		}
	}
}

void CPythonNetworkStream::__LocalizeItemLinks(char* buf, size_t bufSize)
{
	// Replace item names in hyperlinks with client-side localized names
	// Format: |Hitem:VNUM:flags:socket0:socket1:socket2...|h[ItemName]|h

	// Early exit: skip processing if no item links exist
	if (!strstr(buf, "|Hitem:"))
		return;

	char* pSearch = buf;
	char result[2048];
	char* pResult = result;
	char* pResultEnd = result + sizeof(result) - 1;

	while (*pSearch && pResult < pResultEnd)
	{
		// Look for |Hitem:
		if (strncmp(pSearch, "|Hitem:", 7) == 0)
		{
			char* pLinkStart = pSearch;
			pSearch += 7;

			// Extract vnum (first hex value after "item:")
			DWORD dwVnum = 0;
			while (*pSearch && *pSearch != ':' && *pSearch != '|')
			{
				if (*pSearch >= '0' && *pSearch <= '9')
					dwVnum = dwVnum * 16 + (*pSearch - '0');
				else if (*pSearch >= 'a' && *pSearch <= 'f')
					dwVnum = dwVnum * 16 + (*pSearch - 'a' + 10);
				else if (*pSearch >= 'A' && *pSearch <= 'F')
					dwVnum = dwVnum * 16 + (*pSearch - 'A' + 10);
				pSearch++;
			}

			// Find |h[ which marks the start of the item name
			char* pNameStart = strstr(pSearch, "|h[");
			if (pNameStart)
			{
				pNameStart += 3; // Skip "|h["

				// Find ]|h which marks the end of the item name
				char* pNameEnd = strstr(pNameStart, "]|h");
				if (pNameEnd)
				{
					// Get the client-side item name
					CItemData* pItemData;
					const char* szLocalName = NULL;
					if (CItemManager::Instance().GetItemDataPointer(dwVnum, &pItemData))
						szLocalName = pItemData->GetName();

					// Copy everything from link start to |h[
					size_t copyLen = pNameStart - pLinkStart;
					if (pResult + copyLen < pResultEnd)
					{
						memcpy(pResult, pLinkStart, copyLen);
						pResult += copyLen;
					}

					// Insert the localized name (or original if not found)
					const char* szName = szLocalName ? szLocalName : pNameStart;
					size_t nameLen = szLocalName ? strlen(szLocalName) : (pNameEnd - pNameStart);
					if (pResult + nameLen < pResultEnd)
					{
						if (szLocalName)
						{
							memcpy(pResult, szLocalName, nameLen);
						}
						else
						{
							memcpy(pResult, pNameStart, nameLen);
						}
						pResult += nameLen;
					}

					// Copy ]|h
					if (pResult + 3 < pResultEnd)
					{
						memcpy(pResult, "]|h", 3);
						pResult += 3;
					}

					pSearch = pNameEnd + 3; // Skip past ]|h
					continue;
				}
			}

			// If we couldn't parse properly, just copy the character and continue
			if (pResult < pResultEnd)
				*pResult++ = *pLinkStart;
			pSearch = pLinkStart + 1;
		}
		else
		{
			if (pResult < pResultEnd)
				*pResult++ = *pSearch;
			pSearch++;
		}
	}
	*pResult = '\0';

	strncpy(buf, result, bufSize - 1);
	buf[bufSize - 1] = '\0';
}

bool CPythonNetworkStream::RecvChatPacket()
{
	return PhaseGameChatBridge::HandleChat(this);
}

bool CPythonNetworkStream::RecvWhisperPacket()
{
	return PhaseGameChatBridge::HandleWhisper(this);
}

bool CPythonNetworkStream::SendWhisperPacket(const char * name, const char * c_szChat)
{
	return PhaseGameChatBridge::SendWhisper(this, name, c_szChat);
}


bool CPythonNetworkStream::RecvPointChange()
{
	TPacketGCPointChange PointChange;

	if (!Recv(sizeof(TPacketGCPointChange), &PointChange))
	{
		Tracen("Recv Point Change Packet Error");
		return false;
	}

	CPythonCharacterManager& rkChrMgr = CPythonCharacterManager::Instance();
	rkChrMgr.ShowPointEffect(PointChange.Type, PointChange.dwVID);

	CInstanceBase * pInstance = CPythonCharacterManager::Instance().GetMainActorPtr();

	// 자신의 Point가 변경되었을 경우..
	if ((pInstance && PointChange.dwVID == pInstance->GetVirtualID()) ||
		(m_dwMainActorVID != 0 && PointChange.dwVID == m_dwMainActorVID))
	{
		CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
		rkPlayer.SetStatus(PointChange.Type, PointChange.value);

		switch (PointChange.Type)
		{
			case POINT_STAT_RESET_COUNT:
				__RefreshStatus();
				break;
			case POINT_PLAYTIME:
				m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].dwPlayMinutes = PointChange.value;
				break;
			case POINT_LEVEL:
				m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].byLevel = PointChange.value;
				__RefreshStatus();
				__RefreshSkillWindow();
				break;
			case POINT_ST:
			case POINT_DX:
			case POINT_HT:
			case POINT_IQ:
				__RefreshStatus();
				__RefreshSkillWindow();
				break;
			case POINT_SKILL:
			case POINT_SUB_SKILL:
			case POINT_HORSE_SKILL:
				__RefreshSkillWindow();
				break;
			case POINT_ENERGY:
				if (PointChange.value == 0)
				{
					rkPlayer.SetStatus(POINT_ENERGY_END_TIME, 0);
				}
				__RefreshStatus();
				break;
			default:
				__RefreshStatus();
				break;
		}

		if (POINT_GOLD == PointChange.Type)
		{
			if (PointChange.amount > 0)
			{
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnPickMoney", Py_BuildValue("(i)", PointChange.amount));
			}
		}
	}
	else
	{
		if (PointChange.Type == POINT_LEVEL)
		{
			pInstance = CPythonCharacterManager::Instance().GetInstancePtr(PointChange.dwVID);
			if (pInstance)
			{
				pInstance->SetLevel(PointChange.value);
				pInstance->UpdateTextTailLevel(PointChange.value);
			}
		}
	}

	return true;
}

bool CPythonNetworkStream::RecvStunPacket()
{
	TPacketGCStun StunPacket;

	if (!Recv(sizeof(StunPacket), &StunPacket))
	{
		Tracen("CPythonNetworkStream::RecvStunPacket Error");
		return false;
	}

	//Tracef("RecvStunPacket %d\n", StunPacket.vid);

	CPythonCharacterManager& rkChrMgr=CPythonCharacterManager::Instance();
	CInstanceBase * pkInstSel = rkChrMgr.GetInstancePtr(StunPacket.vid);

	if (pkInstSel)
	{
		if (CPythonCharacterManager::Instance().GetMainActorPtr()==pkInstSel)
			pkInstSel->Die();
		else
			pkInstSel->Stun();
	}

	return true;
}

bool CPythonNetworkStream::RecvDeadPacket()
{
	TPacketGCDead DeadPacket;
	if (!Recv(sizeof(DeadPacket), &DeadPacket))
	{
		Tracen("CPythonNetworkStream::RecvDeadPacket Error");
		return false;
	}

	return PhaseGameCombatBridge::HandleDead(this, DeadPacket);
}

bool CPythonNetworkStream::SendCharacterPositionPacket(BYTE iPosition)
{
	TPacketCGPosition PositionPacket;

	PositionPacket.header = CG::CHARACTER_POSITION;
	PositionPacket.length = sizeof(PositionPacket);
	PositionPacket.position = iPosition;

	if (!Send(sizeof(TPacketCGPosition), &PositionPacket))
	{
		Tracen("Send Character Position Packet Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendOnClickPacket(DWORD vid)
{
	TPacketCGOnClick OnClickPacket;
	OnClickPacket.header	= CG::ON_CLICK;
	OnClickPacket.length = sizeof(OnClickPacket);
	OnClickPacket.vid		= vid;

	if (!Send(sizeof(OnClickPacket), &OnClickPacket))
	{
		Tracen("Send On_Click Packet Error");
		return false;
	}

	Tracef("SendOnClickPacket\n");
	return true;
}

bool CPythonNetworkStream::RecvCharacterPositionPacket()
{
	TPacketGCPosition PositionPacket;

	if (!Recv(sizeof(TPacketGCPosition), &PositionPacket))
		return false;

	CInstanceBase * pChrInstance = CPythonCharacterManager::Instance().GetInstancePtr(PositionPacket.vid);

	if (!pChrInstance)
		return true;

	//pChrInstance->UpdatePosition(PositionPacket.position);

	return true;
}

bool CPythonNetworkStream::RecvMotionPacket()
{
	TPacketGCMotion MotionPacket;

	if (!Recv(sizeof(TPacketGCMotion), &MotionPacket))
		return false;

	CInstanceBase * pMainInstance = CPythonCharacterManager::Instance().GetInstancePtr(MotionPacket.vid);
	CInstanceBase * pVictimInstance = NULL;

	if (0 != MotionPacket.victim_vid)
		pVictimInstance = CPythonCharacterManager::Instance().GetInstancePtr(MotionPacket.victim_vid);

	if (!pMainInstance)
		return false;

	return true;
}

bool CPythonNetworkStream::RecvShopPacket()
{
	TPacketGCShop packet_shop;
	if (!Recv(sizeof(packet_shop), &packet_shop))
		return false;

	std::vector<char> buf;
	int iSize = packet_shop.length - sizeof(packet_shop);
	if (iSize > 0)
	{
		buf.resize(iSize);
		if (!Recv(iSize, &buf[0]))
			return false;
	}

	return PhaseGameShopBridge::HandleShop(this, packet_shop, buf);
}

bool CPythonNetworkStream::RecvShopSub_Start(const std::vector<char>& buf)
{
	return PhaseGameShopBridge::HandleShopStart(this, buf);
}

bool CPythonNetworkStream::RecvShopSub_StartEx(const std::vector<char>& buf)
{
	return PhaseGameShopBridge::HandleShopStartEx(this, buf);
}

bool CPythonNetworkStream::RecvShopSub_End(const std::vector<char>& buf)
{
	return PhaseGameShopBridge::HandleShopEnd(this, buf);
}

bool CPythonNetworkStream::RecvShopSub_UpdateItem(const std::vector<char>& buf)
{
	return PhaseGameShopBridge::HandleShopUpdateItem(this, buf);
}

bool CPythonNetworkStream::RecvShopSub_UpdatePrice(const std::vector<char>& buf)
{
	return PhaseGameShopBridge::HandleShopUpdatePrice(this, buf);
}

bool CPythonNetworkStream::RecvShopSub_NotEnoughMoney(const std::vector<char>& /*buf*/)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnShopError", Py_BuildValue("(s)", "NOT_ENOUGH_MONEY"));
	return true;
}

bool CPythonNetworkStream::RecvShopSub_NotEnoughMoneyEx(const std::vector<char>& /*buf*/)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnShopError", Py_BuildValue("(s)", "NOT_ENOUGH_MONEY_EX"));
	return true;
}

bool CPythonNetworkStream::RecvShopSub_Soldout(const std::vector<char>& /*buf*/)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnShopError", Py_BuildValue("(s)", "SOLDOUT"));
	return true;
}

bool CPythonNetworkStream::RecvShopSub_InventoryFull(const std::vector<char>& /*buf*/)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnShopError", Py_BuildValue("(s)", "INVENTORY_FULL"));
	return true;
}

bool CPythonNetworkStream::RecvShopSub_InvalidPos(const std::vector<char>& /*buf*/)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnShopError", Py_BuildValue("(s)", "INVALID_POS"));
	return true;
}

bool CPythonNetworkStream::RecvExchangePacket()
{
	TPacketGCExchange exchange_packet;

	if (!Recv(sizeof(exchange_packet), &exchange_packet))
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&exchange_packet), sizeof(exchange_packet));
	(void)Client::Network::Handlers::RefineExchangeDomainHandler::HandleExchangePacket(span);
	return PhaseGameExchangeBridge::HandleExchange(this, exchange_packet);
}

bool CPythonNetworkStream::RecvExchangeSub_Start(const TPacketGCExchange& pack)
{
	return PhaseGameExchangeBridge::HandleExchangeSub_Start(this, pack);
}

bool CPythonNetworkStream::RecvExchangeSub_ItemAdd(const TPacketGCExchange& pack)
{
	return PhaseGameExchangeBridge::HandleExchangeSub_ItemAdd(this, pack);
}

bool CPythonNetworkStream::RecvExchangeSub_ItemDel(const TPacketGCExchange& pack)
{
	return PhaseGameExchangeBridge::HandleExchangeSub_ItemDel(this, pack);
}

bool CPythonNetworkStream::RecvExchangeSub_ElkAdd(const TPacketGCExchange& pack)
{
	return PhaseGameExchangeBridge::HandleExchangeSub_ElkAdd(this, pack);
}

bool CPythonNetworkStream::RecvExchangeSub_Accept(const TPacketGCExchange& pack)
{
	return PhaseGameExchangeBridge::HandleExchangeSub_Accept(this, pack);
}

bool CPythonNetworkStream::RecvExchangeSub_End(const TPacketGCExchange& pack)
{
	return PhaseGameExchangeBridge::HandleExchangeSub_End(this, pack);
}

bool CPythonNetworkStream::RecvExchangeSub_Already(const TPacketGCExchange& pack)
{
	return PhaseGameExchangeBridge::HandleExchangeSub_Already(this, pack);
}

bool CPythonNetworkStream::RecvExchangeSub_LessElk(const TPacketGCExchange& pack)
{
	return PhaseGameExchangeBridge::HandleExchangeSub_LessElk(this, pack);
}

// Quest - Zaimplementowane w PythonNetworkStreamPhaseGameQuest.cpp
// RequestMakeGuild - Zaimplementowane w PythonNetworkStreamPhaseGameGuild.cpp


void CPythonNetworkStream::ToggleGameDebugInfo()
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "ToggleDebugInfo", Py_BuildValue("()"));
}

bool CPythonNetworkStream::SendExchangeStartPacket(DWORD vid)
{
	if (!__CanActMainInstance())
		return true;

	auto buffer = Client::Network::Handlers::RefineExchangeDomainHandler::EncodeExchangeStart(vid);
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracef("send_trade_start_packet Error\n");
		return false;
	}

	Tracef("send_trade_start_packet   vid %d \n", vid);
	return true;
}

bool CPythonNetworkStream::SendExchangeElkAddPacket(DWORD elk)
{
	if (!__CanActMainInstance())
		return true;

	auto buffer = Client::Network::Handlers::RefineExchangeDomainHandler::EncodeExchangeElkAdd(elk);
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracef("send_trade_elk_add_packet Error\n");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendExchangeItemAddPacket(TItemPos ItemPos, BYTE byDisplayPos)
{
	if (!__CanActMainInstance())
		return true;

	auto buffer = Client::Network::Handlers::RefineExchangeDomainHandler::EncodeExchangeItemAdd(ItemPos, byDisplayPos);
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracef("send_trade_item_add_packet Error\n");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendExchangeItemDelPacket(BYTE pos)
{
	assert(!"Can't be called function - CPythonNetworkStream::SendExchangeItemDelPacket");
	return true;
}

bool CPythonNetworkStream::SendExchangeAcceptPacket()
{
	if (!__CanActMainInstance())
		return true;

	auto buffer = Client::Network::Handlers::RefineExchangeDomainHandler::EncodeExchangeAccept();
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracef("send_trade_accept_packet Error\n");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendExchangeExitPacket()
{
	if (!__CanActMainInstance())
		return true;

	auto buffer = Client::Network::Handlers::RefineExchangeDomainHandler::EncodeExchangeCancel();
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracef("send_trade_exit_packet Error\n");
		return false;
	}

	return true;
}

// PointReset 개임시
bool CPythonNetworkStream::SendPointResetPacket()
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "StartPointReset", Py_BuildValue("()"));
	return true;
}

bool CPythonNetworkStream::__IsPlayerAttacking()
{
	CPythonCharacterManager& rkChrMgr=CPythonCharacterManager::Instance();
	CInstanceBase* pkInstMain=rkChrMgr.GetMainActorPtr();
	if (!pkInstMain)
		return false;

	if (!pkInstMain->IsAttacking())
		return false;

	return true;
}

// Quest/Script zaimplementowane w PythonNetworkStreamPhaseGameQuest.cpp


bool CPythonNetworkStream::RecvSkillCoolTimeEnd()
{
	TPacketGCSkillCoolTimeEnd kPacketSkillCoolTimeEnd;
	if (!Recv(sizeof(kPacketSkillCoolTimeEnd), &kPacketSkillCoolTimeEnd))
	{
		Tracen("CPythonNetworkStream::RecvSkillCoolTimeEnd - RecvError");
		return false;
	}

	return PhaseGameSkillsBridge::HandleSkillCooltimeEnd(this, kPacketSkillCoolTimeEnd);
}

bool CPythonNetworkStream::RecvSkillLevel()
{
	TPacketGCSkillLevel packet;

	if (!Recv(sizeof(TPacketGCSkillLevel), &packet))
	{
		Tracen("CPythonNetworkStream::RecvSkillLevel - RecvError");
		return false;
	}

	return PhaseGameSkillsBridge::HandleSkillLevel(this, packet);
}

bool CPythonNetworkStream::RecvSkillLevelNew()
{
	TPacketGCSkillLevelNew packet;

	if (!Recv(sizeof(TPacketGCSkillLevelNew), &packet))
	{
		Tracen("CPythonNetworkStream::RecvSkillLevelNew - RecvError");
		return false;
	}

	return PhaseGameSkillsBridge::HandleSkillLevelNew(this, packet);
}

bool CPythonNetworkStream::RecvDamageInfoPacket()
{
	TPacketGCDamageInfo DamageInfoPacket;

	if (!Recv(sizeof(TPacketGCDamageInfo), &DamageInfoPacket))
	{
		Tracen("Recv Target Packet Error");
		return false;
	}

	std::span<const uint8_t> payload(reinterpret_cast<const uint8_t*>(&DamageInfoPacket), sizeof(DamageInfoPacket));
	(void)Network::Dispatchers::NetworkStreamPhaseGameBridge::RouteGamePacket(GC::DAMAGE_INFO, payload);

	return PhaseGameCombatBridge::HandleDamageInfo(this, DamageInfoPacket);
}

// RecvTargetPacket zaimplementowane w PythonNetworkStreamPhaseGameTarget.cpp


bool CPythonNetworkStream::RecvMountPacket()
{
	TPacketGCMount MountPacket;

	if (!Recv(sizeof(TPacketGCMount), &MountPacket))
	{
		Tracen("Recv Mount Packet Error");
		return false;
	}

	CInstanceBase * pInstance = CPythonCharacterManager::Instance().GetInstancePtr(MountPacket.vid);

	if (pInstance)
	{
		// Mount
		if (0 != MountPacket.mount_vid)
		{
//			pInstance->Ride(MountPacket.pos, MountPacket.mount_vid);
		}
		// Unmount
		else
		{
//			pInstance->Unride(MountPacket.pos, MountPacket.x, MountPacket.y);
		}
	}

	if (CPythonPlayer::Instance().IsMainCharacterIndex(MountPacket.vid))
	{
//		CPythonPlayer::Instance().SetRidingVehicleIndex(MountPacket.mount_vid);
	}

	return true;
}

// RecvChangeSpeedPacket moved to Network/Handlers/ChangeSpeedHandler.cpp

///////////////////////////////////////////////////////////////////////////////////////////////////
// Recv

bool CPythonNetworkStream::SendAttackPacket(UINT uMotAttack, DWORD dwVIDVictim)
{
	if (!__CanActMainInstance())
		return true;

	// Strangler Fig C++23: Przekierowanie wykonania do nowoczesnej domeny ataku
	(void)Client::Bridge::StranglerFacade::Instance().ExecuteAttack(dwVIDVictim, static_cast<uint8_t>(uMotAttack));

#ifdef ATTACK_TIME_LOG
	static DWORD prevTime = timeGetTime();
	DWORD curTime = timeGetTime();
	TraceError("TIME: %.4f(%.4f) ATTACK_PACKET: %d TARGET: %d", curTime/1000.0f, (curTime-prevTime)/1000.0f, uMotAttack, dwVIDVictim);
	prevTime = curTime;
#endif

	auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	if (pDriver && pDriver->GetDriverName() != "standard_x64")
	{
		Network::Domain::AttackCommand cmd{
			.targetVid = dwVIDVictim,
			.attackerVid = 0,
			.attackType = static_cast<uint8_t>(uMotAttack),
			.sequence = 0,
			.attackMotion = uMotAttack
		};
		auto enc = pDriver->EncodeAttack(cmd);
		if (enc.has_value())
		{
			const auto& buf = enc.value();
			if (!Send(static_cast<int>(buf.size()), buf.data()))
			{
				Tracen("Send Battle Attack Packet Error (UPM)");
				return false;
			}
			return true;
		}
	}

	TPacketCGAttack kPacketAtk;

	kPacketAtk.header = CG::ATTACK;
	kPacketAtk.length = sizeof(kPacketAtk);
	kPacketAtk.bType = uMotAttack;
	kPacketAtk.dwVictimVID = dwVIDVictim;

	if (!SendSpecial(sizeof(kPacketAtk), &kPacketAtk))
	{
		Tracen("Send Battle Attack Packet Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendSpecial(int nLen, void * pvBuf)
{
	uint16_t wHeader = *(uint16_t *) pvBuf;

	switch (wHeader)
	{
		case CG::ATTACK:
			{
				TPacketCGAttack * pkPacketAtk = (TPacketCGAttack *) pvBuf;
				pkPacketAtk->bCRCMagicCubeProcPiece = GetProcessCRCMagicCubePiece();
				pkPacketAtk->bCRCMagicCubeFilePiece = GetProcessCRCMagicCubePiece();
				return Send(nLen, pvBuf);
			}
			break;
	}

	return Send(nLen, pvBuf);
}

// Fly/Targeting zaimplementowane w PythonNetworkStreamPhaseGameTarget.cpp


bool CPythonNetworkStream::SendSyncPositionElementPacket(DWORD dwVictimVID, DWORD dwVictimX, DWORD dwVictimY)
{
	TPacketCGSyncPositionElement kSyncPos;
	kSyncPos.dwVID=dwVictimVID;
	kSyncPos.lX=dwVictimX;
	kSyncPos.lY=dwVictimY;

	__LocalPositionToGlobalPosition(kSyncPos.lX, kSyncPos.lY);

	if (!Send(sizeof(kSyncPos), &kSyncPos))
	{
		Tracen("CPythonNetworkStream::SendSyncPositionElementPacket - ERROR");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::RecvMessenger()
{
	TPacketGCMessenger p;
	if (!Recv(sizeof(p), &p))
		return false;

	return PhaseGameRefineBridge::HandleMessenger(this, p);
}

// Party - Zaimplementowane w PythonNetworkStreamPhaseGameParty.cpp


// Guild - Zaimplementowane w PythonNetworkStreamPhaseGameGuild.cpp


/////////////////////////////////////////////////////////////////////////
// Fishing

// Fishing i Dungeon - Zaimplementowane w PythonNetworkStreamPhaseGameMisc.cpp

// Dungeon
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// MyShop
bool CPythonNetworkStream::SendBuildPrivateShopPacket(const char * c_szName, const std::vector<TShopItemTable> & c_rSellingItemStock)
{
	TPacketCGMyShop packet;
	packet.header = CG::MYSHOP;
	packet.length = sizeof(packet) + sizeof(TShopItemTable) * c_rSellingItemStock.size();
	strncpy(packet.szSign, c_szName, SHOP_SIGN_MAX_LEN);
	packet.bCount = static_cast<unsigned char>(c_rSellingItemStock.size());
	if (!Send(sizeof(packet), &packet))
		return false;

	for (std::vector<TShopItemTable>::const_iterator itor = c_rSellingItemStock.begin(); itor < c_rSellingItemStock.end(); ++itor)
	{
		const TShopItemTable & c_rItem = *itor;
		if (!Send(sizeof(c_rItem), &c_rItem))
			return false;
	}

	return true;
}

bool CPythonNetworkStream::RecvShopSignPacket()
{
	TPacketGCShopSign p;
	if (!Recv(sizeof(TPacketGCShopSign), &p))
		return false;

	return PhaseGameShopBridge::HandleShopSign(this, p);
}
/////////////////////////////////////////////////////////////////////////

// Time - Zaimplementowane w PythonNetworkStreamPhaseGameMisc.cpp



bool CPythonNetworkStream::RecvWalkModePacket()
{
	TPacketGCWalkMode WalkModePacket;
	if (!Recv(sizeof(WalkModePacket), &WalkModePacket))
		return false;

	CInstanceBase * pInstance = CPythonCharacterManager::Instance().GetInstancePtr(WalkModePacket.vid);
	if (pInstance)
	{
		if (WALKMODE_RUN == WalkModePacket.mode)
		{
			pInstance->SetRunMode();
		}
		else
		{
			pInstance->SetWalkMode();
		}
	}

	return true;
}

bool CPythonNetworkStream::RecvChangeSkillGroupPacket()
{
	TPacketGCChangeSkillGroup ChangeSkillGroup;
	if (!Recv(sizeof(ChangeSkillGroup), &ChangeSkillGroup))
		return false;

	m_dwMainActorSkillGroup = ChangeSkillGroup.skill_group;

	CPythonPlayer::Instance().NEW_ClearSkillData();
	__RefreshCharacterWindow();
	return true;
}

void CPythonNetworkStream::__TEST_SetSkillGroupFake(int iIndex)
{
	m_dwMainActorSkillGroup = DWORD(iIndex);

	CPythonPlayer::Instance().NEW_ClearSkillData();
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshCharacter", Py_BuildValue("()"));
}

bool CPythonNetworkStream::SendRefinePacket(BYTE byPos, BYTE byType)
{
	auto buffer = Client::Network::Handlers::RefineExchangeDomainHandler::EncodeRefine(byPos, byType);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	return true;
}

bool CPythonNetworkStream::SendSelectItemPacket(DWORD dwItemPos)
{
	auto buffer = Client::Network::Handlers::QuestDialogDomainHandler::EncodeScriptSelectItem(dwItemPos);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvRefineInformationPacket()
{
	TPacketGCRefineInformation kRefineInfoPacket;
	if (!Recv(sizeof(kRefineInfoPacket), &kRefineInfoPacket))
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&kRefineInfoPacket), sizeof(kRefineInfoPacket));
	(void)Client::Network::Handlers::RefineExchangeDomainHandler::HandleRefineInformation(span);
	return PhaseGameRefineBridge::HandleRefineInformation(this, kRefineInfoPacket);
}

bool CPythonNetworkStream::RecvRefineInformationPacketNew()
{
	TPacketGCRefineInformationNew kRefineInfoPacket;
	if (!Recv(sizeof(kRefineInfoPacket), &kRefineInfoPacket))
		return false;

	std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(&kRefineInfoPacket), sizeof(kRefineInfoPacket));
	(void)Client::Network::Handlers::RefineExchangeDomainHandler::HandleRefineInformationNew(span);
	return PhaseGameRefineBridge::HandleRefineInformationNew(this, kRefineInfoPacket);
}

// RecvNPCList moved to Network/Handlers/NpcPositionHandler.cpp

bool CPythonNetworkStream::__SendCRCReportPacket()
{
	return true;
}

bool CPythonNetworkStream::SendClientVersionPacket()
{
	std::string filename;
	GetExcutedFileName(filename);
	filename = CFileNameHelper::NoPath(filename);
	CFileNameHelper::ChangeDosPath(filename);

	auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	if (!pDriver)
	{
		Network::Protocol::ProtocolDriverRegistry::Instance().InitializeDefaults();
		pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	}

	auto verRes = pDriver->EncodeClientVersion(filename);
	if (!verRes.has_value())
	{
		Tracef("SendClientVersionPacket: EncodeClientVersion failed");
		return false;
	}

	const auto& verData = verRes.value();
	if (!Send(static_cast<int>(verData.size()), verData.data()))
	{
		Tracef("SendClientReportPacket Error");
		return false;
	}

	__SendInternalBuffer();
	return true;
}

bool CPythonNetworkStream::RecvAffectAddPacket()
{
	TPacketGCAffectAdd kAffectAdd;

	if (!Recv(sizeof(kAffectAdd), &kAffectAdd))
		return false;

	TPacketAffectElement & rkElement = kAffectAdd.elem;

	if (rkElement.bPointIdxApplyOn == POINT_ENERGY)
	{
		CPythonPlayer::instance().SetStatus (POINT_ENERGY_END_TIME, CPythonApplication::Instance().GetServerTimeStamp() + rkElement.lDuration);
		__RefreshStatus();
	}

	// MR-16: Classic affect duration countdown
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_NEW_AddAffect", Py_BuildValue("(iiiii)", rkElement.dwType, rkElement.bPointIdxApplyOn, rkElement.lApplyValue, rkElement.lDuration, rkElement.dwFlag));
	// MR-16: -- END OF -- Classic affect duration countdown

	return true;
}

// RecvAffectRemovePacket moved to Network/Handlers/AffectRemoveHandler.cpp

bool CPythonNetworkStream::RecvChannelPacket()
{
	TPacketGCChannel kChannelPacket;
	if (!Recv(sizeof(kChannelPacket), &kChannelPacket))
		return false;

	return PhaseGameWorldBridge::HandleChannel(this, kChannelPacket);
}

bool CPythonNetworkStream::RecvViewEquipPacket()
{
	TPacketGCViewEquip kViewEquipPacket;
	if (!Recv(sizeof(kViewEquipPacket), &kViewEquipPacket))
		return false;

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OpenEquipmentDialog", Py_BuildValue("(i)", kViewEquipPacket.dwVID));

	for (int i = 0; i < WEAR_MAX_NUM; ++i)
	{
		TEquipmentItemSet & rItemSet = kViewEquipPacket.equips[i];
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "SetEquipmentDialogItem", Py_BuildValue("(iiii)", kViewEquipPacket.dwVID, i, rItemSet.vnum, rItemSet.count));

		for (int j = 0; j < ITEM_SOCKET_SLOT_MAX_NUM; ++j)
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "SetEquipmentDialogSocket", Py_BuildValue("(iiii)", kViewEquipPacket.dwVID, i, j, rItemSet.alSockets[j]));

		for (int k = 0; k < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++k)
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "SetEquipmentDialogAttr", Py_BuildValue("(iiiii)", kViewEquipPacket.dwVID, i, k, rItemSet.aAttr[k].bType, rItemSet.aAttr[k].sValue));
	}

	return true;
}

bool CPythonNetworkStream::RecvLandPacket()
{
	TPacketGCLandList kLandList;
	if (!Recv(sizeof(kLandList), &kLandList))
		return false;

	std::vector<DWORD> kVec_dwGuildID;

	CPythonMiniMap & rkMiniMap = CPythonMiniMap::Instance();
	CPythonBackground & rkBG = CPythonBackground::Instance();
	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();

	rkMiniMap.ClearGuildArea();
	rkBG.ClearGuildArea();

	int iPacketSize = (kLandList.length - sizeof(TPacketGCLandList));
	for (; iPacketSize > 0; iPacketSize-=sizeof(TLandPacketElement))
	{
		TLandPacketElement kElement;
		if (!Recv(sizeof(TLandPacketElement), &kElement))
			return false;

		rkMiniMap.RegisterGuildArea(kElement.dwID,
									kElement.dwGuildID,
									kElement.x,
									kElement.y,
									kElement.width,
									kElement.height);

		if (pMainInstance)
		if (kElement.dwGuildID == pMainInstance->GetGuildID())
		{
			rkBG.RegisterGuildArea(kElement.x,
								   kElement.y,
								   kElement.x+kElement.width,
								   kElement.y+kElement.height);
		}

		if (0 != kElement.dwGuildID)
			kVec_dwGuildID.push_back(kElement.dwGuildID);
	}

	__DownloadSymbol(kVec_dwGuildID);

	return true;
}

// Target Create/Update/Delete zaimplementowane w PythonNetworkStreamPhaseGameTarget.cpp


bool CPythonNetworkStream::RecvLoverInfoPacket()
{
	TPacketGCLoverInfo kLoverInfo;
	if (!Recv(sizeof(kLoverInfo), &kLoverInfo))
		return false;

	return PhaseGameRefineBridge::HandleLoverInfo(this, kLoverInfo);
}

bool CPythonNetworkStream::RecvLovePointUpdatePacket()
{
	TPacketGCLovePointUpdate kLovePointUpdate;
	if (!Recv(sizeof(kLovePointUpdate), &kLovePointUpdate))
		return false;

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_UpdateLovePoint", Py_BuildValue("(i)", kLovePointUpdate.byLovePoint));
#ifdef _DEBUG
	Tracef("RECV LOVE POINT UPDATE : %d\n", kLovePointUpdate.byLovePoint);
#endif
	return true;
}

// DigMotion - Zaimplementowane w PythonNetworkStreamPhaseGameMisc.cpp



// 용혼석 강화
bool CPythonNetworkStream::SendDragonSoulRefinePacket(BYTE bRefineType, TItemPos* pos)
{
	auto buffer = Client::Network::Handlers::RefineExchangeDomainHandler::EncodeDragonSoulRefine(bRefineType, pos, DS_REFINE_WINDOW_MAX_NUM);
	if (!Send(buffer.size(), buffer.data()))
	{
		return false;
	}
	return true;
}


#if defined(ENABLE_DISCORD_RPC)
#include "discord_rpc.h"
#include "Discord.h"
#ifdef _DEBUG
#pragma comment(lib, "discord_rpc_d.lib")
#else
#pragma comment(lib, "discord_rpc_r.lib")
#endif

static int64_t StartTime = 0;
void CPythonNetworkStream::Discord_Start()
{
	StartTime = time(0);
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	Discord_Initialize(Discord::DiscordClientID, &handlers, 1, nullptr);
	Discord_Update(false);
}

void CPythonNetworkStream::Discord_Update(const bool bInGame)
{
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.startTimestamp = StartTime;

	if (!bInGame) {
		Discord_UpdatePresence(&discordPresence);
		return;
	}

	/*Name*/
	auto NameData = Discord::GetNameData();
	discordPresence.state = std::get<0>(NameData).c_str();
	discordPresence.details = std::get<1>(NameData).c_str();

	/*Race*/
	auto RaceData = Discord::GetRaceData();
	discordPresence.largeImageKey = std::get<0>(RaceData).c_str();
	discordPresence.largeImageText = std::get<1>(RaceData).c_str();

	/*Empire*/
	auto EmpireData = Discord::GetEmpireData();
	discordPresence.smallImageKey = std::get<0>(EmpireData).c_str();
	discordPresence.smallImageText = std::get<1>(EmpireData).c_str();

	Discord_UpdatePresence(&discordPresence);
}

void CPythonNetworkStream::Discord_Close()
{
	Discord_Shutdown();
}
#endif
