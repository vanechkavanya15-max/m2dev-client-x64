#include "StdAfx.h"
#include <span>
#include "PythonNetworkStream.h"
#include "Packet.h"
#include "Network/Dispatchers/NetworkStreamPhaseGameBridge.h"
#include "GuildMarkDownloader.h"
#include "MarkManager.h"

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


bool CPythonNetworkStream::RecvObserverAddPacket()
{
	TPacketGCObserverAdd kObserverAddPacket;
	if (!Recv(sizeof(kObserverAddPacket), &kObserverAddPacket))
		return false;

	CPythonMiniMap::Instance().AddObserver(
		kObserverAddPacket.vid, 
		kObserverAddPacket.x*100.0f, 
		kObserverAddPacket.y*100.0f);

	return true;
}

bool CPythonNetworkStream::RecvObserverRemovePacket()
{
	TPacketGCObserverRemove kObserverRemovePacket;
	if (!Recv(sizeof(kObserverRemovePacket), &kObserverRemovePacket))
		return false;

	CPythonMiniMap::Instance().RemoveObserver(
		kObserverRemovePacket.vid
	);

	return true;
}

bool CPythonNetworkStream::RecvObserverMovePacket()
{
	TPacketGCObserverMove kObserverMovePacket;
	if (!Recv(sizeof(kObserverMovePacket), &kObserverMovePacket))
		return false;

	CPythonMiniMap::Instance().MoveObserver(
		kObserverMovePacket.vid, 
		kObserverMovePacket.x*100.0f, 
		kObserverMovePacket.y*100.0f);

	return true;
}


bool CPythonNetworkStream::RecvWarpPacket()
{
	return PhaseGameSyncBridge::HandleWarp(this);
}

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
	NANOBEGIN
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
	
	assert(kStatePacket.lX >= 0 && kStatePacket.lX < 204800);

	__LocalPositionToGlobalPosition(kStatePacket.lX, kStatePacket.lY);

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
	NANOEND
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
	if (pInstance)
	if (PointChange.dwVID == pInstance->GetVirtualID())
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

bool CPythonNetworkStream::RecvQuestInfoPacket()
{
	TPacketGCQuestInfo QuestInfo;

	if (!Peek(sizeof(TPacketGCQuestInfo), &QuestInfo))
	{
		Tracen("Recv Quest Info Packet Error #1");
		return false;
	}

	if (!Peek(QuestInfo.length))
	{
		Tracen("Recv Quest Info Packet Error #2");
		return false;
	}

	Recv(sizeof(TPacketGCQuestInfo));

	return PhaseGameQuestBridge::HandleQuestInfo(this, QuestInfo);
}

bool CPythonNetworkStream::RecvQuestConfirmPacket()
{
	TPacketGCQuestConfirm kQuestConfirmPacket;
	if (!Recv(sizeof(kQuestConfirmPacket), &kQuestConfirmPacket))
	{
		Tracen("RecvQuestConfirmPacket Error");
		return false;
	}

	return PhaseGameQuestBridge::HandleQuestConfirm(this, kQuestConfirmPacket);
}

bool CPythonNetworkStream::RecvRequestMakeGuild()
{
	TPacketGCBlank blank;
	if (!Recv(sizeof(blank), &blank))
	{
		Tracen("RecvRequestMakeGuild Packet Error");
		return false;
	}

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "AskGuildName", Py_BuildValue("()"));

	return true;
}

void CPythonNetworkStream::ToggleGameDebugInfo()
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "ToggleDebugInfo", Py_BuildValue("()"));
}

bool CPythonNetworkStream::SendExchangeStartPacket(DWORD vid)
{
	if (!__CanActMainInstance())
		return true;

	TPacketCGExchange	packet;

	packet.header		= CG::EXCHANGE;
	packet.length = sizeof(packet);
	packet.subheader	= ExchangeSub::CG::START;
	packet.arg1			= vid;

	if (!Send(sizeof(packet), &packet))
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

	TPacketCGExchange	packet;

	packet.header		= CG::EXCHANGE;
	packet.length = sizeof(packet);
	packet.subheader	= ExchangeSub::CG::ELK_ADD;
	packet.arg1			= elk;

	if (!Send(sizeof(packet), &packet))
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

	TPacketCGExchange	packet;

	packet.header		= CG::EXCHANGE;
	packet.length = sizeof(packet);
	packet.subheader	= ExchangeSub::CG::ITEM_ADD;
	packet.Pos			= ItemPos;
	packet.arg2			= byDisplayPos;

	if (!Send(sizeof(packet), &packet))
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

	if (!__CanActMainInstance())
		return true;

	TPacketCGExchange	packet;

	packet.header		= CG::EXCHANGE;
	packet.length = sizeof(packet);
	packet.subheader	= ExchangeSub::CG::ITEM_DEL;
	packet.arg1			= pos;

	if (!Send(sizeof(packet), &packet))
	{
		Tracef("send_trade_item_del_packet Error\n");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendExchangeAcceptPacket()
{
	if (!__CanActMainInstance())
		return true;
	
	TPacketCGExchange	packet;

	packet.header		= CG::EXCHANGE;
	packet.length = sizeof(packet);
	packet.subheader	= ExchangeSub::CG::ACCEPT;

	if (!Send(sizeof(packet), &packet))
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

	TPacketCGExchange	packet;

	packet.header		= CG::EXCHANGE;
	packet.length = sizeof(packet);
	packet.subheader	= ExchangeSub::CG::CANCEL;

	if (!Send(sizeof(packet), &packet))
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

bool CPythonNetworkStream::RecvScriptPacket()
{
	TPacketGCScript ScriptPacket;

	if (!Recv(sizeof(TPacketGCScript), &ScriptPacket))
	{
		TraceError("RecvScriptPacket_RecvError");
		return false;
	}

	if (ScriptPacket.length < sizeof(TPacketGCScript))
	{
		TraceError("RecvScriptPacket_SizeError");
		return false;
	}

	ScriptPacket.length -= sizeof(TPacketGCScript);
	
	static std::string str;
	str = "";
	str.resize(ScriptPacket.length+1);

	if (!Recv(ScriptPacket.length, &str[0]))
		return false;

	str[str.size()-1] = '\0';
	
	int iIndex = CPythonEventManager::Instance().RegisterEventSetFromString(str);

	if (-1 != iIndex)
	{
		CPythonEventManager::Instance().SetVisibleLineCount(iIndex, 30);
		CPythonNetworkStream::Instance().OnScriptEventStart(ScriptPacket.skin,iIndex);
	}

	return true;
}

bool CPythonNetworkStream::SendScriptAnswerPacket(int iAnswer)
{
	TPacketCGScriptAnswer ScriptAnswer;

	ScriptAnswer.header = CG::SCRIPT_ANSWER;
	ScriptAnswer.length = sizeof(ScriptAnswer);
	ScriptAnswer.answer = (BYTE) iAnswer;
	if (!Send(sizeof(TPacketCGScriptAnswer), &ScriptAnswer))
	{
		Tracen("Send Script Answer Packet Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendScriptButtonPacket(unsigned int iIndex)
{
	TPacketCGScriptButton ScriptButton;

	ScriptButton.header = CG::SCRIPT_BUTTON;
	ScriptButton.length = sizeof(ScriptButton);
	ScriptButton.idx = iIndex;
	if (!Send(sizeof(TPacketCGScriptButton), &ScriptButton))
	{
		Tracen("Send Script Button Packet Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendAnswerMakeGuildPacket(const char * c_szName)
{
	TPacketCGAnswerMakeGuild Packet;

	Packet.header = CG::ANSWER_MAKE_GUILD;
	Packet.length = sizeof(Packet);
	strncpy(Packet.guild_name, c_szName, GUILD_NAME_MAX_LEN);
	Packet.guild_name[GUILD_NAME_MAX_LEN] = '\0';

	if (!Send(sizeof(Packet), &Packet))
	{
		Tracen("SendAnswerMakeGuild Packet Error");
		return false;
	}

// 	Tracef(" SendAnswerMakeGuildPacket : %s", c_szName);
	return true;
}

// SendQuestInputStringPacket moved to Network/Handlers/QuestInputStringHandler.cpp

bool CPythonNetworkStream::SendQuestConfirmPacket(BYTE byAnswer, DWORD dwPID)
{
	TPacketCGQuestConfirm kPacket;
	kPacket.header = CG::QUEST_CONFIRM;
	kPacket.length = sizeof(kPacket);
	kPacket.answer = byAnswer;
	kPacket.requestPID = dwPID;

	if (!Send(sizeof(kPacket), &kPacket))
	{
		Tracen("SendQuestConfirmPacket Error");
		return false;
	}

	Tracenf(" SendQuestConfirmPacket : %d, %d", byAnswer, dwPID);
	return true;
}

bool CPythonNetworkStream::SendQuestCancelPacket()
{
	TPacketCGQuestCancel Packet;
	Packet.header = CG::QUEST_CANCEL;
	Packet.length = sizeof(Packet);

	if (!Send(sizeof(Packet), &Packet))
	{
		Tracen("SendQuestCancelPacket Error");
		return false;
	}

	Tracenf(" SendQuestCancelPacket");
	return true;
}

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

bool CPythonNetworkStream::RecvTargetPacket()
{
	TPacketGCTarget TargetPacket;

	if (!Recv(sizeof(TPacketGCTarget), &TargetPacket))
	{
		Tracen("Recv Target Packet Error");
		return false;
	}

	CInstanceBase * pInstPlayer = CPythonCharacterManager::Instance().GetMainActorPtr();
	CInstanceBase * pInstTarget = CPythonCharacterManager::Instance().GetInstancePtr(TargetPacket.dwVID);
	if (pInstPlayer && pInstTarget)
	{
		if (!pInstTarget->IsDead())
		{
			if (pInstTarget->IsPC() || pInstTarget->IsBuilding())
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "CloseTargetBoardIfDifferent", Py_BuildValue("(i)", TargetPacket.dwVID));
			else if (pInstPlayer->CanViewTargetHP(*pInstTarget))
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "SetHPTargetBoard", Py_BuildValue("(ii)", TargetPacket.dwVID, TargetPacket.bHPPercent));
			else
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "CloseTargetBoard", Py_BuildValue("()"));

			m_pInstTarget = pInstTarget;
		}
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "CloseTargetBoard", Py_BuildValue("()"));
	}

	return true;
}

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

bool CPythonNetworkStream::RecvAddFlyTargetingPacket()
{
	TPacketGCFlyTargeting kPacket;
	if (!Recv(sizeof(kPacket), &kPacket))
		return false;

	__GlobalPositionToLocalPosition(kPacket.lX, kPacket.lY);

	Tracef("VID [%d]가 타겟을 추가 설정\n",kPacket.dwShooterVID);

	CPythonCharacterManager & rpcm = CPythonCharacterManager::Instance();

	CInstanceBase * pShooter = rpcm.GetInstancePtr(kPacket.dwShooterVID);

	if (!pShooter)
	{
#ifndef _DEBUG		
		TraceError("CPythonNetworkStream::RecvFlyTargetingPacket() - dwShooterVID[%d] NOT EXIST", kPacket.dwShooterVID);
#endif
		return true;
	}

	CInstanceBase * pTarget = rpcm.GetInstancePtr(kPacket.dwTargetVID);

	if (kPacket.dwTargetVID && pTarget)
	{
		pShooter->GetGraphicThingInstancePtr()->AddFlyTarget(pTarget->GetGraphicThingInstancePtr());
	}
	else
	{
		float h = CPythonBackground::Instance().GetHeight(kPacket.lX,kPacket.lY) + 60.0f; // TEMPORARY HEIGHT
		pShooter->GetGraphicThingInstancePtr()->AddFlyTarget(D3DXVECTOR3(kPacket.lX,kPacket.lY,h));
		//pShooter->GetGraphicThingInstancePtr()->SetFlyTarget(kPacket.kPPosTarget.x,kPacket.kPPosTarget.y,);
	}

	return true;
}

bool CPythonNetworkStream::RecvFlyTargetingPacket()
{
	TPacketGCFlyTargeting kPacket;
	if (!Recv(sizeof(kPacket), &kPacket))
		return false;

	__GlobalPositionToLocalPosition(kPacket.lX, kPacket.lY);

	//Tracef("CPythonNetworkStream::RecvFlyTargetingPacket - VID [%d]\n",kPacket.dwShooterVID);

	CPythonCharacterManager & rpcm = CPythonCharacterManager::Instance();

	CInstanceBase * pShooter = rpcm.GetInstancePtr(kPacket.dwShooterVID);

	if (!pShooter)
	{
#ifdef _DEBUG
		TraceError("CPythonNetworkStream::RecvFlyTargetingPacket() - dwShooterVID[%d] NOT EXIST", kPacket.dwShooterVID);
#endif
		return true;
	}

	CInstanceBase * pTarget = rpcm.GetInstancePtr(kPacket.dwTargetVID);

	if (kPacket.dwTargetVID && pTarget)
	{
		pShooter->GetGraphicThingInstancePtr()->SetFlyTarget(pTarget->GetGraphicThingInstancePtr());
	}
	else
	{
		float h = CPythonBackground::Instance().GetHeight(kPacket.lX, kPacket.lY) + 60.0f; // TEMPORARY HEIGHT
		pShooter->GetGraphicThingInstancePtr()->SetFlyTarget(D3DXVECTOR3(kPacket.lX,kPacket.lY,h));
		//pShooter->GetGraphicThingInstancePtr()->SetFlyTarget(kPacket.kPPosTarget.x,kPacket.kPPosTarget.y,);
	}

	return true;
}

bool CPythonNetworkStream::SendShootPacket(UINT uSkill)
{
	TPacketCGShoot kPacketShoot;
	kPacketShoot.header=CG::SHOOT;
	kPacketShoot.length = sizeof(kPacketShoot);
	kPacketShoot.bType=uSkill;

	if (!Send(sizeof(kPacketShoot), &kPacketShoot))
	{
		Tracen("SendShootPacket Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendAddFlyTargetingPacket(DWORD dwTargetVID, const TPixelPosition & kPPosTarget)
{
	TPacketCGFlyTargeting packet;

	//CPythonCharacterManager & rpcm = CPythonCharacterManager::Instance();

	packet.header	= CG::ADD_FLY_TARGETING;
	packet.length = sizeof(packet);
	packet.dwTargetVID = dwTargetVID;
	packet.lX = kPPosTarget.x;
	packet.lY = kPPosTarget.y;

	__LocalPositionToGlobalPosition(packet.lX, packet.lY);
	
	if (!Send(sizeof(packet), &packet))
	{
		Tracen("Send FlyTargeting Packet Error");
		return false;
	}

	return true;
}


bool CPythonNetworkStream::SendFlyTargetingPacket(DWORD dwTargetVID, const TPixelPosition & kPPosTarget)
{
	TPacketCGFlyTargeting packet;

	//CPythonCharacterManager & rpcm = CPythonCharacterManager::Instance();

	packet.header	= CG::FLY_TARGETING;
	packet.length = sizeof(packet);
	packet.dwTargetVID = dwTargetVID;
	packet.lX = kPPosTarget.x;
	packet.lY = kPPosTarget.y;

	__LocalPositionToGlobalPosition(packet.lX, packet.lY);
	
	if (!Send(sizeof(packet), &packet))
	{
		Tracen("Send FlyTargeting Packet Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::RecvCreateFlyPacket()
{
	TPacketGCCreateFly kPacket;
	if (!Recv(sizeof(TPacketGCCreateFly), &kPacket))
		return false;

	return PhaseGameTargetBridge::HandleCreateFly(this, kPacket);
}

bool CPythonNetworkStream::SendTargetPacket(DWORD dwVID)
{
	TPacketCGTarget packet;
	packet.header = CG::TARGET;
	packet.length = sizeof(packet);
	packet.dwVID = dwVID;

	if (!Send(sizeof(packet), &packet))
	{
		Tracen("Send Target Packet Error");
		return false;
	}

	return true;
}

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

//////////////////////////////////////////////////////////////////////////
// Party

bool CPythonNetworkStream::SendPartyInvitePacket(DWORD dwVID)
{
	TPacketCGPartyInvite kPartyInvitePacket;
	kPartyInvitePacket.header = CG::PARTY_INVITE;
	kPartyInvitePacket.length = sizeof(kPartyInvitePacket);
	kPartyInvitePacket.vid = dwVID;

	if (!Send(sizeof(kPartyInvitePacket), &kPartyInvitePacket))
	{
		Tracenf("CPythonNetworkStream::SendPartyInvitePacket [%ud] - PACKET SEND ERROR", dwVID);
		return false;
	}

	Tracef(" << SendPartyInvitePacket : %d\n", dwVID);
	return true;
}

bool CPythonNetworkStream::SendPartyInviteAnswerPacket(DWORD dwLeaderVID, BYTE byAnswer)
{
	TPacketCGPartyInviteAnswer kPartyInviteAnswerPacket;
	kPartyInviteAnswerPacket.header = CG::PARTY_INVITE_ANSWER;
	kPartyInviteAnswerPacket.length = sizeof(kPartyInviteAnswerPacket);
	kPartyInviteAnswerPacket.leader_pid = dwLeaderVID;
	kPartyInviteAnswerPacket.accept = byAnswer;

	if (!Send(sizeof(kPartyInviteAnswerPacket), &kPartyInviteAnswerPacket))
	{
		Tracenf("CPythonNetworkStream::SendPartyInviteAnswerPacket [%ud %ud] - PACKET SEND ERROR", dwLeaderVID, byAnswer);
		return false;
	}

	Tracef(" << SendPartyInviteAnswerPacket : %d, %d\n", dwLeaderVID, byAnswer);
	return true;
}

bool CPythonNetworkStream::SendPartyRemovePacket(DWORD dwPID)
{
	TPacketCGPartyRemove kPartyInviteRemove;
	kPartyInviteRemove.header = CG::PARTY_REMOVE;
	kPartyInviteRemove.length = sizeof(kPartyInviteRemove);
	kPartyInviteRemove.pid = dwPID;

	if (!Send(sizeof(kPartyInviteRemove), &kPartyInviteRemove))
	{
		Tracenf("CPythonNetworkStream::SendPartyRemovePacket [%ud] - PACKET SEND ERROR", dwPID);
		return false;
	}

	Tracef(" << SendPartyRemovePacket : %d\n", dwPID);
	return true;
}

bool CPythonNetworkStream::SendPartySetStatePacket(DWORD dwVID, BYTE byState, BYTE byFlag)
{
	TPacketCGPartySetState kPartySetState;
	kPartySetState.header = CG::PARTY_SET_STATE;
	kPartySetState.length = sizeof(kPartySetState);
	kPartySetState.dwVID = dwVID;
	kPartySetState.byState = byState;
	kPartySetState.byFlag = byFlag;

	if (!Send(sizeof(kPartySetState), &kPartySetState))
	{
		Tracenf("CPythonNetworkStream::SendPartySetStatePacket(%ud, %ud) - PACKET SEND ERROR", dwVID, byState);
		return false;
	}

	Tracef(" << SendPartySetStatePacket : %d, %d, %d\n", dwVID, byState, byFlag);
	return true;
}

bool CPythonNetworkStream::SendPartyUseSkillPacket(BYTE bySkillIndex, DWORD dwVID)
{
	TPacketCGPartyUseSkill kPartyUseSkill;
	kPartyUseSkill.header = CG::PARTY_USE_SKILL;
	kPartyUseSkill.length = sizeof(kPartyUseSkill);
	kPartyUseSkill.bySkillIndex = bySkillIndex;
	kPartyUseSkill.dwTargetVID = dwVID;

	if (!Send(sizeof(kPartyUseSkill), &kPartyUseSkill))
	{
		Tracenf("CPythonNetworkStream::SendPartyUseSkillPacket(%ud, %ud) - PACKET SEND ERROR", bySkillIndex, dwVID);
		return false;
	}

	Tracef(" << SendPartyUseSkillPacket : %d, %d\n", bySkillIndex, dwVID);
	return true;
}

bool CPythonNetworkStream::SendPartyParameterPacket(BYTE byDistributeMode)
{
	TPacketCGPartyParameter kPartyParameter;
	kPartyParameter.header = CG::PARTY_PARAMETER;
	kPartyParameter.length = sizeof(kPartyParameter);
	kPartyParameter.bDistributeMode = byDistributeMode;

	if (!Send(sizeof(kPartyParameter), &kPartyParameter))
	{
		Tracenf("CPythonNetworkStream::SendPartyParameterPacket(%d) - PACKET SEND ERROR", byDistributeMode);
		return false;
	}

	Tracef(" << SendPartyParameterPacket : %d\n", byDistributeMode);
	return true;
}

bool CPythonNetworkStream::RecvPartyInvite()
{
	TPacketGCPartyInvite kPartyInvitePacket;
	if (!Recv(sizeof(kPartyInvitePacket), &kPartyInvitePacket))
		return false;

	return PhaseGamePartyBridge::HandlePartyInvite(this, kPartyInvitePacket);
}

bool CPythonNetworkStream::RecvPartyAdd()
{
	TPacketGCPartyAdd kPartyAddPacket;
	if (!Recv(sizeof(kPartyAddPacket), &kPartyAddPacket))
		return false;

	return PhaseGamePartyBridge::HandlePartyAdd(this, kPartyAddPacket);
}

bool CPythonNetworkStream::RecvPartyUpdate()
{
	TPacketGCPartyUpdate kPartyUpdatePacket;
	if (!Recv(sizeof(kPartyUpdatePacket), &kPartyUpdatePacket))
		return false;

	return PhaseGamePartyBridge::HandlePartyUpdate(this, kPartyUpdatePacket);
}

bool CPythonNetworkStream::RecvPartyRemove()
{
	TPacketGCPartyRemove kPartyRemovePacket;
	if (!Recv(sizeof(kPartyRemovePacket), &kPartyRemovePacket))
		return false;

	return PhaseGamePartyBridge::HandlePartyRemove(this, kPartyRemovePacket);
}

bool CPythonNetworkStream::RecvPartyLink()
{
	TPacketGCPartyLink kPartyLinkPacket;
	if (!Recv(sizeof(kPartyLinkPacket), &kPartyLinkPacket))
		return false;

	CPythonPlayer::Instance().LinkPartyMember(kPartyLinkPacket.pid, kPartyLinkPacket.vid);
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "LinkPartyMember", Py_BuildValue("(ii)", kPartyLinkPacket.pid, kPartyLinkPacket.vid));
	Tracef(" >> RecvPartyLink : %d, %d\n", kPartyLinkPacket.pid, kPartyLinkPacket.vid);

	return true;
}

bool CPythonNetworkStream::RecvPartyUnlink()
{
	TPacketGCPartyUnlink kPartyUnlinkPacket;
	if (!Recv(sizeof(kPartyUnlinkPacket), &kPartyUnlinkPacket))
		return false;

	CPythonPlayer::Instance().UnlinkPartyMember(kPartyUnlinkPacket.pid);

	if (CPythonPlayer::Instance().IsMainCharacterIndex(kPartyUnlinkPacket.vid))
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "UnlinkAllPartyMember", Py_BuildValue("()"));
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "UnlinkPartyMember", Py_BuildValue("(i)", kPartyUnlinkPacket.pid));
	}

	Tracef(" >> RecvPartyUnlink : %d, %d\n", kPartyUnlinkPacket.pid, kPartyUnlinkPacket.vid);

	return true;
}

bool CPythonNetworkStream::RecvPartyParameter()
{
	TPacketGCPartyParameter kPartyParameterPacket;
	if (!Recv(sizeof(kPartyParameterPacket), &kPartyParameterPacket))
		return false;

	return PhaseGamePartyBridge::HandlePartyParameter(this, kPartyParameterPacket);
}

// Party
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// Guild

bool CPythonNetworkStream::SendGuildAddMemberPacket(DWORD dwVID)
{
	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket) + sizeof(dwVID);
	GuildPacket.bySubHeader = GuildSub::CG::ADD_MEMBER;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;
	if (!Send(sizeof(dwVID), &dwVID))
		return false;

	Tracef(" SendGuildAddMemberPacket\n", dwVID);
	return true;
}

bool CPythonNetworkStream::SendGuildRemoveMemberPacket(DWORD dwPID)
{
	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket) + sizeof(dwPID);
	GuildPacket.bySubHeader = GuildSub::CG::REMOVE_MEMBER;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;
	if (!Send(sizeof(dwPID), &dwPID))
		return false;

	Tracef(" SendGuildRemoveMemberPacket %d\n", dwPID);
	return true;
}

bool CPythonNetworkStream::SendGuildChangeGradeNamePacket(BYTE byGradeNumber, const char * c_szName)
{
	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket) + sizeof(BYTE) + (GUILD_GRADE_NAME_MAX_LEN + 1);
	GuildPacket.bySubHeader = GuildSub::CG::CHANGE_GRADE_NAME;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;
	if (!Send(sizeof(byGradeNumber), &byGradeNumber))
		return false;

	char szName[GUILD_GRADE_NAME_MAX_LEN+1];
	strncpy(szName, c_szName, GUILD_GRADE_NAME_MAX_LEN);
	szName[GUILD_GRADE_NAME_MAX_LEN] = '\0';

	if (!Send(sizeof(szName), &szName))
		return false;

	Tracef(" SendGuildChangeGradeNamePacket %d, %s\n", byGradeNumber, c_szName);
	return true;
}

bool CPythonNetworkStream::SendGuildChangeGradeAuthorityPacket(BYTE byGradeNumber, BYTE byAuthority)
{
	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket) + sizeof(BYTE) + sizeof(BYTE);
	GuildPacket.bySubHeader = GuildSub::CG::CHANGE_GRADE_AUTHORITY;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;
	if (!Send(sizeof(byGradeNumber), &byGradeNumber))
		return false;
	if (!Send(sizeof(byAuthority), &byAuthority))
		return false;

	Tracef(" SendGuildChangeGradeAuthorityPacket %d, %d\n", byGradeNumber, byAuthority);
	return true;
}

bool CPythonNetworkStream::SendGuildOfferPacket(DWORD dwExperience)
{
	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket) + sizeof(dwExperience);
	GuildPacket.bySubHeader = GuildSub::CG::OFFER;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;
	if (!Send(sizeof(dwExperience), &dwExperience))
		return false;

	Tracef(" SendGuildOfferPacket %d\n", dwExperience);
	return true;
}

bool CPythonNetworkStream::SendGuildPostCommentPacket(const char * c_szMessage)
{
	TPacketCGGuild GuildPacket;
	BYTE bySize = BYTE(strlen(c_szMessage)) + 1;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket) + sizeof(BYTE) + bySize;
	GuildPacket.bySubHeader = GuildSub::CG::POST_COMMENT;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;

	if (!Send(sizeof(bySize), &bySize))
		return false;
	if (!Send(bySize, c_szMessage))
		return false;

	Tracef(" SendGuildPostCommentPacket %d, %s\n", bySize, c_szMessage);
	return true;
}

bool CPythonNetworkStream::SendGuildDeleteCommentPacket(DWORD dwIndex)
{
	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket) + sizeof(dwIndex);
	GuildPacket.bySubHeader = GuildSub::CG::DELETE_COMMENT;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;

	if (!Send(sizeof(dwIndex), &dwIndex))
		return false;

	Tracef(" SendGuildDeleteCommentPacket %d\n", dwIndex);
	return true;
}

bool CPythonNetworkStream::SendGuildRefreshCommentsPacket(DWORD dwHighestIndex)
{
	static DWORD s_LastTime = timeGetTime() - 1001;

	if (timeGetTime() - s_LastTime < 1000)
		return true;
	s_LastTime = timeGetTime();

	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket);
	GuildPacket.bySubHeader = GuildSub::CG::REFRESH_COMMENT;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;

	Tracef(" SendGuildRefreshCommentPacket %d\n", dwHighestIndex);
	return true;
}

bool CPythonNetworkStream::SendGuildChangeMemberGradePacket(DWORD dwPID, BYTE byGrade)
{
	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket) + sizeof(dwPID) + sizeof(byGrade);
	GuildPacket.bySubHeader = GuildSub::CG::CHANGE_MEMBER_GRADE;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;

	if (!Send(sizeof(dwPID), &dwPID))
		return false;
	if (!Send(sizeof(byGrade), &byGrade))
		return false;

	Tracef(" SendGuildChangeMemberGradePacket %d, %d\n", dwPID, byGrade);
	return true;
}

bool CPythonNetworkStream::SendGuildUseSkillPacket(DWORD dwSkillID, DWORD dwTargetVID)
{
	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket);
	GuildPacket.bySubHeader = GuildSub::CG::USE_SKILL;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;

	if (!Send(sizeof(dwSkillID), &dwSkillID))
		return false;
	if (!Send(sizeof(dwTargetVID), &dwTargetVID))
		return false;

	Tracef(" SendGuildUseSkillPacket %d, %d\n", dwSkillID, dwTargetVID);
	return true;
}

bool CPythonNetworkStream::SendGuildChangeMemberGeneralPacket(DWORD dwPID, BYTE byFlag)
{
	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket) + sizeof(dwPID) + sizeof(byFlag);
	GuildPacket.bySubHeader = GuildSub::CG::CHANGE_MEMBER_GENERAL;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;

	if (!Send(sizeof(dwPID), &dwPID))
		return false;
	if (!Send(sizeof(byFlag), &byFlag))
		return false;

	Tracef(" SendGuildChangeMemberGeneralFlagPacket %d, %d\n", dwPID, byFlag);
	return true;
}

bool CPythonNetworkStream::SendGuildInviteAnswerPacket(DWORD dwGuildID, BYTE byAnswer)
{
	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket) + sizeof(dwGuildID) + sizeof(byAnswer);
	GuildPacket.bySubHeader = GuildSub::CG::GUILD_INVITE_ANSWER;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;

	if (!Send(sizeof(dwGuildID), &dwGuildID))
		return false;
	if (!Send(sizeof(byAnswer), &byAnswer))
		return false;

	Tracef(" SendGuildInviteAnswerPacket %d, %d\n", dwGuildID, byAnswer);
	return true;
}

bool CPythonNetworkStream::SendGuildChargeGSPPacket(DWORD dwMoney)
{
	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket) + sizeof(dwMoney);
	GuildPacket.bySubHeader = GuildSub::CG::CHARGE_GSP;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;

	if (!Send(sizeof(dwMoney), &dwMoney))
		return false;

	Tracef(" SendGuildChargeGSPPacket %d\n", dwMoney);
	return true;
}

bool CPythonNetworkStream::SendGuildDepositMoneyPacket(DWORD dwMoney)
{
	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket) + sizeof(dwMoney);
	GuildPacket.bySubHeader = GuildSub::CG::DEPOSIT_MONEY;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;
	if (!Send(sizeof(dwMoney), &dwMoney))
		return false;

	Tracef(" SendGuildDepositMoneyPacket %d\n", dwMoney);
	return true;
}

bool CPythonNetworkStream::SendGuildWithdrawMoneyPacket(DWORD dwMoney)
{
	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket) + sizeof(dwMoney);
	GuildPacket.bySubHeader = GuildSub::CG::WITHDRAW_MONEY;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;
	if (!Send(sizeof(dwMoney), &dwMoney))
		return false;

	Tracef(" SendGuildWithdrawMoneyPacket %d\n", dwMoney);
	return true;
}

bool CPythonNetworkStream::RecvGuild()
{
	TPacketGCGuild GuildPacket;
	if (!Recv(sizeof(GuildPacket), &GuildPacket))
		return false;

	static const std::unordered_map<uint8_t, bool (CPythonNetworkStream::*)(const TPacketGCGuild&)> handlers = {
		{ GuildSub::GC::LOGIN,                  &CPythonNetworkStream::RecvGuildSub_Login },
		{ GuildSub::GC::LOGOUT,                 &CPythonNetworkStream::RecvGuildSub_Logout },
		{ GuildSub::GC::REMOVE,                 &CPythonNetworkStream::RecvGuildSub_Remove },
		{ GuildSub::GC::LIST,                   &CPythonNetworkStream::RecvGuildSub_List },
		{ GuildSub::GC::GRADE,                  &CPythonNetworkStream::RecvGuildSub_Grade },
		{ GuildSub::GC::GRADE_NAME,             &CPythonNetworkStream::RecvGuildSub_GradeName },
		{ GuildSub::GC::GRADE_AUTH,             &CPythonNetworkStream::RecvGuildSub_GradeAuth },
		{ GuildSub::GC::INFO,                   &CPythonNetworkStream::RecvGuildSub_Info },
		{ GuildSub::GC::COMMENTS,               &CPythonNetworkStream::RecvGuildSub_Comments },
		{ GuildSub::GC::CHANGE_EXP,             &CPythonNetworkStream::RecvGuildSub_ChangeExp },
		{ GuildSub::GC::CHANGE_MEMBER_GRADE,    &CPythonNetworkStream::RecvGuildSub_ChangeMemberGrade },
		{ GuildSub::GC::SKILL_INFO,             &CPythonNetworkStream::RecvGuildSub_SkillInfo },
		{ GuildSub::GC::CHANGE_MEMBER_GENERAL,  &CPythonNetworkStream::RecvGuildSub_ChangeMemberGeneral },
		{ GuildSub::GC::GUILD_INVITE,           &CPythonNetworkStream::RecvGuildSub_Invite },
		{ GuildSub::GC::WAR,                    &CPythonNetworkStream::RecvGuildSub_War },
		{ GuildSub::GC::GUILD_NAME,             &CPythonNetworkStream::RecvGuildSub_Name },
		{ GuildSub::GC::GUILD_WAR_LIST,         &CPythonNetworkStream::RecvGuildSub_WarList },
		{ GuildSub::GC::GUILD_WAR_END_LIST,     &CPythonNetworkStream::RecvGuildSub_WarEndList },
		{ GuildSub::GC::WAR_POINT,              &CPythonNetworkStream::RecvGuildSub_WarPoint },
		{ GuildSub::GC::MONEY_CHANGE,           &CPythonNetworkStream::RecvGuildSub_MoneyChange },
	};

	auto it = handlers.find(GuildPacket.subheader);
	if (it == handlers.end())
	{
		TraceError("RecvGuild: unknown subheader %d", GuildPacket.subheader);
		return true;
	}
	return (this->*(it->second))(GuildPacket);
}

bool CPythonNetworkStream::RecvGuildSub_Login(const TPacketGCGuild& pack)
{
	uint32_t dwPID;
	if (!Recv(sizeof(uint32_t), &dwPID))
		return false;

	// Messenger
	CPythonGuild::TGuildMemberData * pGuildMemberData;
	if (CPythonGuild::Instance().GetMemberDataPtrByPID(dwPID, &pGuildMemberData))
		if (0 != pGuildMemberData->strName.compare(CPythonPlayer::Instance().GetName()))
			CPythonMessenger::Instance().LoginGuildMember(pGuildMemberData->strName.c_str());

	//Tracef(" <Login> %d\n", dwPID);
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_Logout(const TPacketGCGuild& pack)
{
	uint32_t dwPID;
	if (!Recv(sizeof(uint32_t), &dwPID))
		return false;

	// Messenger
	CPythonGuild::TGuildMemberData * pGuildMemberData;
	if (CPythonGuild::Instance().GetMemberDataPtrByPID(dwPID, &pGuildMemberData))
		if (0 != pGuildMemberData->strName.compare(CPythonPlayer::Instance().GetName()))
			CPythonMessenger::Instance().LogoutGuildMember(pGuildMemberData->strName.c_str());

	//Tracef(" <Logout> %d\n", dwPID);
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_Remove(const TPacketGCGuild& pack)
{
	uint32_t dwPID;
	if (!Recv(sizeof(dwPID), &dwPID))
		return false;

	// Main Player 일 경우 DeleteGuild
	if (CPythonGuild::Instance().IsMainPlayer(dwPID))
	{
		CPythonGuild::Instance().Destroy();
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "DeleteGuild", Py_BuildValue("()"));
		CPythonMessenger::Instance().RemoveAllGuildMember();
		__SetGuildID(0);
		__RefreshMessengerWindow();
		__RefreshTargetBoard();
		__RefreshCharacterWindow();
	}
	else
	{
		// Get Member Name
		std::string strMemberName = "";
		CPythonGuild::TGuildMemberData * pData;
		if (CPythonGuild::Instance().GetMemberDataPtrByPID(dwPID, &pData))
		{
			strMemberName = pData->strName;
			CPythonMessenger::Instance().RemoveGuildMember(pData->strName.c_str());
		}

		CPythonGuild::Instance().RemoveMember(dwPID);

		// Refresh
		__RefreshTargetBoardByName(strMemberName.c_str());
		__RefreshGuildWindowMemberPage();
	}

	Tracef(" <Remove> %d\n", dwPID);
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_List(const TPacketGCGuild& pack)
{
	int iPacketSize = int(pack.length) - sizeof(pack);

	for (; iPacketSize > 0;)
	{
		TPacketGCGuildSubMember memberPacket;
		if (!Recv(sizeof(memberPacket), &memberPacket))
			return false;

		char szName[CHARACTER_NAME_MAX_LEN+1] = "";
		if (memberPacket.byNameFlag)
		{
			if (!Recv(sizeof(szName), &szName))
				return false;

			iPacketSize -= CHARACTER_NAME_MAX_LEN+1;
		}
		else
		{
			CPythonGuild::TGuildMemberData * pMemberData;
			if (CPythonGuild::Instance().GetMemberDataPtrByPID(memberPacket.pid, &pMemberData))
			{
				strncpy(szName, pMemberData->strName.c_str(), CHARACTER_NAME_MAX_LEN);
			}
		}

		//Tracef(" <List> %d : %s, %d (%d, %d, %d)\n", memberPacket.pid, szName, memberPacket.byGrade, memberPacket.byJob, memberPacket.byLevel, memberPacket.dwOffer);

		CPythonGuild::SGuildMemberData GuildMemberData;
		GuildMemberData.dwPID = memberPacket.pid;
		GuildMemberData.byGrade = memberPacket.byGrade;
		GuildMemberData.strName = szName;
		GuildMemberData.byJob = memberPacket.byJob;
		GuildMemberData.byLevel = memberPacket.byLevel;
		GuildMemberData.dwOffer = memberPacket.dwOffer;
		GuildMemberData.byGeneralFlag = memberPacket.byIsGeneral;
		CPythonGuild::Instance().RegisterMember(GuildMemberData);

		// Messenger
		if (strcmp(szName, CPythonPlayer::Instance().GetName()))
			CPythonMessenger::Instance().AppendGuildMember(szName);

		__RefreshTargetBoardByName(szName);

		iPacketSize -= sizeof(memberPacket);
	}

	__RefreshGuildWindowInfoPage();
	__RefreshGuildWindowMemberPage();
	__RefreshMessengerWindow();
	__RefreshCharacterWindow();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_Grade(const TPacketGCGuild& pack)
{
	BYTE byCount;
	if (!Recv(sizeof(byCount), &byCount))
		return false;

	for (BYTE i = 0; i < byCount; ++ i)
	{
		BYTE byIndex;
		if (!Recv(sizeof(byCount), &byIndex))
			return false;
		TPacketGCGuildSubGrade GradePacket;
		if (!Recv(sizeof(GradePacket), &GradePacket))
			return false;

		auto data = CPythonGuild::SGuildGradeData(GradePacket.auth_flag, GradePacket.grade_name);
		CPythonGuild::Instance().SetGradeData(byIndex, data);
		//Tracef(" <Grade> [%d/%d] : %s, %d\n", byIndex, byCount, GradePacket.grade_name, GradePacket.auth_flag);
	}
	__RefreshGuildWindowGradePage();
	__RefreshGuildWindowMemberPageGradeComboBox();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_GradeName(const TPacketGCGuild& pack)
{
	BYTE byGradeNumber;
	if (!Recv(sizeof(byGradeNumber), &byGradeNumber))
		return false;

	char szGradeName[GUILD_GRADE_NAME_MAX_LEN+1] = "";
	if (!Recv(sizeof(szGradeName), &szGradeName))
		return false;

	CPythonGuild::Instance().SetGradeName(byGradeNumber, szGradeName);
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildGrade", Py_BuildValue("()"));

	Tracef(" <Change Grade Name> %d, %s\n", byGradeNumber, szGradeName);
	__RefreshGuildWindowGradePage();
	__RefreshGuildWindowMemberPageGradeComboBox();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_GradeAuth(const TPacketGCGuild& pack)
{
	BYTE byGradeNumber;
	if (!Recv(sizeof(byGradeNumber), &byGradeNumber))
		return false;
	BYTE byAuthorityFlag;
	if (!Recv(sizeof(byAuthorityFlag), &byAuthorityFlag))
		return false;

	CPythonGuild::Instance().SetGradeAuthority(byGradeNumber, byAuthorityFlag);
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildGrade", Py_BuildValue("()"));

	Tracef(" <Change Grade Authority> %d, %d\n", byGradeNumber, byAuthorityFlag);
	__RefreshGuildWindowGradePage();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_Info(const TPacketGCGuild& pack)
{
	TPacketGCGuildInfo GuildInfo;
	if (!Recv(sizeof(GuildInfo), &GuildInfo))
		return false;

	if (!PhaseGameGuildBridge::HandleGuildSub_Info(GuildInfo))
		return false;

	__RefreshGuildWindowInfoPage();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_Comments(const TPacketGCGuild& pack)
{
	BYTE byCount;
	if (!Recv(sizeof(byCount), &byCount))
		return false;

	CPythonGuild::Instance().ClearComment();
	//Tracef(" >>> Comments Count : %d\n", byCount);

	for (BYTE i = 0; i < byCount; ++i)
	{
		DWORD dwCommentID;
		if (!Recv(sizeof(dwCommentID), &dwCommentID))
			return false;

		char szName[CHARACTER_NAME_MAX_LEN+1] = "";
		if (!Recv(sizeof(szName), &szName))
			return false;

		char szComment[GULID_COMMENT_MAX_LEN+1] = "";
		if (!Recv(sizeof(szComment), &szComment))
			return false;

		//Tracef(" [Comment-%d] : %s, %s\n", dwCommentID, szName, szComment);
		CPythonGuild::Instance().RegisterComment(dwCommentID, szName, szComment);
	}

	__RefreshGuildWindowBoardPage();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_ChangeExp(const TPacketGCGuild& pack)
{
	BYTE byLevel;
	if (!Recv(sizeof(byLevel), &byLevel))
		return false;
	uint32_t dwEXP;
	if (!Recv(sizeof(dwEXP), &dwEXP))
		return false;
	CPythonGuild::Instance().SetGuildEXP(byLevel, dwEXP);
	Tracef(" <ChangeEXP> %d, %d\n", byLevel, dwEXP);
	__RefreshGuildWindowInfoPage();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_ChangeMemberGrade(const TPacketGCGuild& pack)
{
	uint32_t dwPID;
	if (!Recv(sizeof(dwPID), &dwPID))
		return false;
	BYTE byGrade;
	if (!Recv(sizeof(byGrade), &byGrade))
		return false;
	CPythonGuild::Instance().ChangeGuildMemberGrade(dwPID, byGrade);
	Tracef(" <ChangeMemberGrade> %d, %d\n", dwPID, byGrade);
	__RefreshGuildWindowMemberPage();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_SkillInfo(const TPacketGCGuild& pack)
{
	CPythonGuild::TGuildSkillData & rSkillData = CPythonGuild::Instance().GetGuildSkillDataRef();
	if (!Recv(sizeof(rSkillData.bySkillPoint), &rSkillData.bySkillPoint))
		return false;
	if (!Recv(sizeof(rSkillData.bySkillLevel), rSkillData.bySkillLevel))
		return false;
	if (!Recv(sizeof(rSkillData.wGuildPoint), &rSkillData.wGuildPoint))
		return false;
	if (!Recv(sizeof(rSkillData.wMaxGuildPoint), &rSkillData.wMaxGuildPoint))
		return false;

	Tracef(" <SkillInfo> %d / %d, %d\n", rSkillData.bySkillPoint, rSkillData.wGuildPoint, rSkillData.wMaxGuildPoint);
	__RefreshGuildWindowSkillPage();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_ChangeMemberGeneral(const TPacketGCGuild& pack)
{
	uint32_t dwPID;
	if (!Recv(sizeof(dwPID), &dwPID))
		return false;
	BYTE byFlag;
	if (!Recv(sizeof(byFlag), &byFlag))
		return false;

	CPythonGuild::Instance().ChangeGuildMemberGeneralFlag(dwPID, byFlag);
	Tracef(" <ChangeMemberGeneralFlag> %d, %d\n", dwPID, byFlag);
	__RefreshGuildWindowMemberPage();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_Invite(const TPacketGCGuild& pack)
{
	uint32_t dwGuildID;
	if (!Recv(sizeof(dwGuildID), &dwGuildID))
		return false;
	char szGuildName[GUILD_NAME_MAX_LEN+1];
	if (!Recv(GUILD_NAME_MAX_LEN, &szGuildName))
		return false;

	szGuildName[GUILD_NAME_MAX_LEN] = 0;

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RecvGuildInviteQuestion", Py_BuildValue("(is)", dwGuildID, szGuildName));
	Tracef(" <Guild Invite> %d, %s\n", dwGuildID, szGuildName);
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_War(const TPacketGCGuild& pack)
{
	TPacketGCGuildWar kGuildWar;
	if (!Recv(sizeof(kGuildWar), &kGuildWar))
		return false;

	switch (kGuildWar.bWarState)
	{
		case GUILD_WAR_SEND_DECLARE:
			Tracef(" >> GuildSub::GC::WAR : GUILD_WAR_SEND_DECLARE\n");
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
				"BINARY_GuildWar_OnSendDeclare",
				Py_BuildValue("(i)", kGuildWar.dwGuildOpp)
			);
			break;
		case GUILD_WAR_RECV_DECLARE:
			Tracef(" >> GuildSub::GC::WAR : GUILD_WAR_RECV_DECLARE\n");
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
				"BINARY_GuildWar_OnRecvDeclare",
				Py_BuildValue("(ii)", kGuildWar.dwGuildOpp, kGuildWar.bType)
			);
			break;
		case GUILD_WAR_ON_WAR:
			Tracef(" >> GuildSub::GC::WAR : GUILD_WAR_ON_WAR : %d, %d\n", kGuildWar.dwGuildSelf, kGuildWar.dwGuildOpp);
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
				"BINARY_GuildWar_OnStart",
				Py_BuildValue("(ii)", kGuildWar.dwGuildSelf, kGuildWar.dwGuildOpp)
			);
			CPythonGuild::Instance().StartGuildWar(kGuildWar.dwGuildOpp);
			break;
		case GUILD_WAR_END:
			Tracef(" >> GuildSub::GC::WAR : GUILD_WAR_END\n");
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
				"BINARY_GuildWar_OnEnd",
				Py_BuildValue("(ii)", kGuildWar.dwGuildSelf, kGuildWar.dwGuildOpp)
			);
			CPythonGuild::Instance().EndGuildWar(kGuildWar.dwGuildOpp);
			break;
	}
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_Name(const TPacketGCGuild& pack)
{
	uint32_t dwID;
	char szGuildName[GUILD_NAME_MAX_LEN+1];

	int iPacketSize = int(pack.length) - sizeof(pack);

	int nItemSize = sizeof(dwID) + GUILD_NAME_MAX_LEN;

	assert(iPacketSize%nItemSize==0 && "GuildSub::GC::GUILD_NAME");

	for (; iPacketSize > 0;)
	{
		if (!Recv(sizeof(dwID), &dwID))
			return false;

		if (!Recv(GUILD_NAME_MAX_LEN, &szGuildName))
			return false;

		szGuildName[GUILD_NAME_MAX_LEN] = 0;

		//Tracef(" >> GulidName [%d : %s]\n", dwID, szGuildName);
		CPythonGuild::Instance().RegisterGuildName(dwID, szGuildName);
		iPacketSize -= nItemSize;
	}
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_WarList(const TPacketGCGuild& pack)
{
	uint32_t dwSrcGuildID;
	uint32_t dwDstGuildID;

	int iPacketSize = int(pack.length) - sizeof(pack);
	int nItemSize = sizeof(dwSrcGuildID) + sizeof(dwDstGuildID);

	assert(iPacketSize%nItemSize==0 && "GuildSub::GC::GUILD_WAR_LIST");

	for (; iPacketSize > 0;)
	{
		if (!Recv(sizeof(dwSrcGuildID), &dwSrcGuildID))
			return false;

		if (!Recv(sizeof(dwDstGuildID), &dwDstGuildID))
			return false;

		Tracef(" >> GulidWarList [%d vs %d]\n", dwSrcGuildID, dwDstGuildID);
		CInstanceBase::InsertGVGKey(dwSrcGuildID, dwDstGuildID);
		CPythonCharacterManager::Instance().ChangeGVG(dwSrcGuildID, dwDstGuildID);
		iPacketSize -= nItemSize;
	}
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_WarEndList(const TPacketGCGuild& pack)
{
	uint32_t dwSrcGuildID;
	uint32_t dwDstGuildID;

	int iPacketSize = int(pack.length) - sizeof(pack);
	int nItemSize = sizeof(dwSrcGuildID) + sizeof(dwDstGuildID);

	assert(iPacketSize%nItemSize==0 && "GuildSub::GC::GUILD_WAR_END_LIST");

	for (; iPacketSize > 0;)
	{

		if (!Recv(sizeof(dwSrcGuildID), &dwSrcGuildID))
			return false;

		if (!Recv(sizeof(dwDstGuildID), &dwDstGuildID))
			return false;

		Tracef(" >> GulidWarEndList [%d vs %d]\n", dwSrcGuildID, dwDstGuildID);
		CInstanceBase::RemoveGVGKey(dwSrcGuildID, dwDstGuildID);
		CPythonCharacterManager::Instance().ChangeGVG(dwSrcGuildID, dwDstGuildID);
		iPacketSize -= nItemSize;
	}
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_WarPoint(const TPacketGCGuild& pack)
{
	TPacketGuildWarPoint GuildWarPoint;
	if (!Recv(sizeof(GuildWarPoint), &GuildWarPoint))
		return false;

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
		"BINARY_GuildWar_OnRecvPoint",
		Py_BuildValue("(iii)", GuildWarPoint.dwGainGuildID, GuildWarPoint.dwOpponentGuildID, GuildWarPoint.lPoint)
	);
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_MoneyChange(const TPacketGCGuild& pack)
{
	uint32_t dwMoney;
	if (!Recv(sizeof(dwMoney), &dwMoney))
		return false;

	CPythonGuild::Instance().SetGuildMoney(dwMoney);

	__RefreshGuildWindowInfoPage();
	Tracef(" >> Guild Money Change : %d\n", dwMoney);
	return true;
}

static DWORD gs_dwMarkUpdateRequestTime = 0;

bool CPythonNetworkStream::RecvMarkUpdate()
{
	TPacketGCMarkUpdate packet;
	if (!Recv(sizeof(packet), &packet))
		return false;

	Tracef(" >> RecvMarkUpdate: guildID=%u, imgIdx=%u\n", packet.guildID, packet.imgIdx);

	CGuildMarkManager::Instance().ReloadMarkImage(packet.imgIdx);

	// Rate limit mark downloads to prevent connection spam from multiple simultaneous guild uploads
	// Allow at most one download request per 1 second from server-pushed updates
	DWORD dwCurrentTime = ELTimer_GetMSec();
	if (dwCurrentTime < gs_dwMarkUpdateRequestTime)
		return true;

	gs_dwMarkUpdateRequestTime = dwCurrentTime + 1000; // 1 second cooldown

	CGuildMarkDownloader& rkGuildMarkDownloader = CGuildMarkDownloader::Instance();
	rkGuildMarkDownloader.Connect(m_kMarkAuth.m_kNetAddr, m_kMarkAuth.m_dwHandle, m_kMarkAuth.m_dwRandomKey);

	return true;
}

// Guild
//////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
// Fishing

bool CPythonNetworkStream::SendFishingPacket(int iRotation)
{
	TPacketCGFishing pack;
	pack.header = CG::FISHING;
	pack.length = sizeof(pack);
	pack.dir = iRotation / 5;

	if (!Send(sizeof(pack), &pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendGiveItemPacket(DWORD dwTargetVID, TItemPos ItemPos, int iItemCount)
{
	TPacketCGGiveItem GiveItemPacket;
	GiveItemPacket.header = CG::ITEM_GIVE;
	GiveItemPacket.length = sizeof(GiveItemPacket);
	GiveItemPacket.dwTargetVID = dwTargetVID;
	GiveItemPacket.ItemPos = ItemPos;
	GiveItemPacket.byItemCount = iItemCount;

	if (!Send(sizeof(GiveItemPacket), &GiveItemPacket))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvFishing()
{
	TPacketGCFishing FishingPacket;
	if (!Recv(sizeof(FishingPacket), &FishingPacket))
		return false;

	return PhaseGameWorldBridge::HandleFishing(this, FishingPacket);
}
// Fishing
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// Dungeon
bool CPythonNetworkStream::RecvDungeon()
{
	TPacketGCDungeon DungeonPacket;
	if (!Recv(sizeof(DungeonPacket), &DungeonPacket))
		return false;

	return PhaseGameWorldBridge::HandleDungeon(this, DungeonPacket);
}
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

bool CPythonNetworkStream::RecvTimePacket()
{
	return PhaseGameSyncBridge::HandleTime(this);
}

bool CPythonNetworkStream::RecvTimeStatusPacket()
{
	return PhaseGameSyncBridge::HandleTimeStatus(this);
}


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
	TPacketCGRefine kRefinePacket;
	kRefinePacket.header = CG::REFINE;
	kRefinePacket.length = sizeof(kRefinePacket);
	kRefinePacket.pos = byPos;
	kRefinePacket.type = byType;

	if (!Send(sizeof(kRefinePacket), &kRefinePacket))
		return false;

	return true;
}

bool CPythonNetworkStream::SendSelectItemPacket(DWORD dwItemPos)
{
	TPacketCGScriptSelectItem kScriptSelectItem;
	kScriptSelectItem.header = CG::SCRIPT_SELECT_ITEM;
	kScriptSelectItem.length = sizeof(kScriptSelectItem);
	kScriptSelectItem.selection = dwItemPos;

	if (!Send(sizeof(kScriptSelectItem), &kScriptSelectItem))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvRefineInformationPacket()
{
	TPacketGCRefineInformation kRefineInfoPacket;
	if (!Recv(sizeof(kRefineInfoPacket), &kRefineInfoPacket))
		return false;

	return PhaseGameRefineBridge::HandleRefineInformation(this, kRefineInfoPacket);
}

bool CPythonNetworkStream::RecvRefineInformationPacketNew()
{
	TPacketGCRefineInformationNew kRefineInfoPacket;
	if (!Recv(sizeof(kRefineInfoPacket), &kRefineInfoPacket))
		return false;

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

	TPacketCGClientVersion kVersionPacket{};
	kVersionPacket.header = CG::CLIENT_VERSION;
	kVersionPacket.length = sizeof(kVersionPacket);

	strncpy(kVersionPacket.filename, filename.c_str(), sizeof(kVersionPacket.filename) - 1);
	kVersionPacket.filename[sizeof(kVersionPacket.filename) - 1] = '\0';

	snprintf(kVersionPacket.timestamp, sizeof(kVersionPacket.timestamp), "%u", 1215955205u);

	if (!Send(sizeof(kVersionPacket), &kVersionPacket))
		Tracef("SendClientReportPacket Error");

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

bool CPythonNetworkStream::RecvTargetCreatePacket()
{
	TPacketGCTargetCreate kTargetCreate;
	if (!Recv(sizeof(kTargetCreate), &kTargetCreate))
		return false;

	return PhaseGameTargetBridge::HandleTargetCreate(this, kTargetCreate);
}

bool CPythonNetworkStream::RecvTargetCreatePacketNew()
{
	TPacketGCTargetCreateNew kTargetCreate;
	if (!Recv(sizeof(kTargetCreate), &kTargetCreate))
		return false;

	CPythonMiniMap & rkpyMiniMap = CPythonMiniMap::Instance();
	CPythonBackground & rkpyBG = CPythonBackground::Instance();
	if (CREATE_TARGET_TYPE_LOCATION == kTargetCreate.byType)
	{
		rkpyMiniMap.CreateTarget(kTargetCreate.lID, kTargetCreate.szTargetName);
	}
	else
	{
		rkpyMiniMap.CreateTarget(kTargetCreate.lID, kTargetCreate.szTargetName, kTargetCreate.dwVID);
		rkpyBG.CreateTargetEffect(kTargetCreate.lID, kTargetCreate.dwVID);
	}

//#ifdef _DEBUG
//	char szBuf[256+1];
//	_snprintf(szBuf, sizeof(szBuf), "캐릭터 타겟이 생성 되었습니다 [%d:%s:%d]", kTargetCreate.lID, kTargetCreate.szTargetName, kTargetCreate.dwVID);
//	CPythonChat::Instance().AppendChat(CHAT_TYPE_NOTICE, szBuf);
//	Tracef(" >> RecvTargetCreatePacketNew %d : %d/%d\n", kTargetCreate.lID, kTargetCreate.byType, kTargetCreate.dwVID);
//#endif

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_OpenAtlasWindow", Py_BuildValue("()"));
	return true;
}

bool CPythonNetworkStream::RecvTargetUpdatePacket()
{
	TPacketGCTargetUpdate kTargetUpdate;
	if (!Recv(sizeof(kTargetUpdate), &kTargetUpdate))
		return false;

	return PhaseGameTargetBridge::HandleTargetUpdate(this, kTargetUpdate);
}

bool CPythonNetworkStream::RecvTargetDeletePacket()
{
	TPacketGCTargetDelete kTargetDelete;
	if (!Recv(sizeof(kTargetDelete), &kTargetDelete))
		return false;

	return PhaseGameTargetBridge::HandleTargetDelete(this, kTargetDelete);
}

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

bool CPythonNetworkStream::RecvDigMotionPacket()
{
	TPacketGCDigMotion kDigMotion;
	if (!Recv(sizeof(kDigMotion), &kDigMotion))
		return false;

#ifdef _DEBUG
	Tracef(" Dig Motion [%d/%d]\n", kDigMotion.vid, kDigMotion.count);
#endif

	IAbstractCharacterManager& rkChrMgr=IAbstractCharacterManager::GetSingleton();
	CInstanceBase * pkInstMain = rkChrMgr.GetInstancePtr(kDigMotion.vid);
	CInstanceBase * pkInstTarget = rkChrMgr.GetInstancePtr(kDigMotion.target_vid);
	if (NULL == pkInstMain)
		return true;

	if (pkInstTarget)
		pkInstMain->NEW_LookAtDestInstance(*pkInstTarget);

	for (int i = 0; i < kDigMotion.count; ++i)
		pkInstMain->PushOnceMotion(CRaceMotionData::NAME_DIG);

	return true;
}


// 용혼석 강화
bool CPythonNetworkStream::SendDragonSoulRefinePacket(BYTE bRefineType, TItemPos* pos)
{
	TPacketCGDragonSoulRefine pk;
	pk.header = CG::DRAGON_SOUL_REFINE;
	pk.length = sizeof(pk);
	pk.bSubType = bRefineType;
	memcpy (pk.ItemGrid, pos, sizeof (TItemPos) * DS_REFINE_WINDOW_MAX_NUM);
	if (!Send(sizeof (pk), &pk))
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
