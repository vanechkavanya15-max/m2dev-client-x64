#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "Packet.h"
#include "PythonApplication.h"
#include "NetworkActorManager.h"
#include "AbstractPlayer.h"
#include "PackLib/PackManager.h"
#include "Client/Network/Protocol/ProtocolDriverRegistry.h"
#include "Client/Network/Protocol/LuriaProtocol.h"

void CPythonNetworkStream::EnableChatInsultFilter(bool isEnable)
{
	m_isEnableChatInsultFilter=isEnable;
}

void CPythonNetworkStream::__FilterInsult(char* /*szLine*/, UINT /*uLineLen*/)
{
}

bool CPythonNetworkStream::IsChatInsultIn(const char* /*c_szMsg*/)
{
	return false;
}

bool CPythonNetworkStream::IsInsultIn(const char* /*c_szMsg*/)
{
	return false;
}

bool CPythonNetworkStream::LoadInsultList(const char* /*c_szInsultListFileName*/)
{
	return true;
}

bool CPythonNetworkStream::LoadConvertTable(DWORD dwEmpireID, const char* c_szFileName)
{
	if (dwEmpireID<1 || dwEmpireID>=4)
		return false;

	TPackFile file;
	if (!CPackManager::Instance().GetFile(c_szFileName, file))
		return false;

	DWORD dwEngCount=26;
	DWORD dwHanCount=(0xc8-0xb0+1)*(0xfe-0xa1+1);
	DWORD dwHanSize=dwHanCount*2;
	DWORD dwFileSize=dwEngCount*2+dwHanSize;

	if (file.size()<dwFileSize)
		return false;

	char* pcData=(char*)file.data();

	STextConvertTable& rkTextConvTable=m_aTextConvTable[dwEmpireID-1];		
	memcpy(rkTextConvTable.acUpper, pcData, dwEngCount);pcData+=dwEngCount;
	memcpy(rkTextConvTable.acLower, pcData, dwEngCount);pcData+=dwEngCount;
	memcpy(rkTextConvTable.aacHan, pcData, dwHanSize);

	return true;
}

// Loading ---------------------------------------------------------------------------
void CPythonNetworkStream::LoadingPhase()
{
	while (m_strPhase == "Loading" && !m_isStartGame && DispatchPacket(m_loadingHandlers))
		;
}

void CPythonNetworkStream::SetLoadingPhase()
{
	if ("Loading"!=m_strPhase)
		m_phaseLeaveFunc.Run();

	Tracef("[PHASE] Entering phase: Loading\n");
	Tracen("");
	Tracen("## Network - Loading Phase ##");
	Tracen("");

	m_strPhase = "Loading";
	m_bEnterGameSent = false;
	RefreshProtocolHandlers();

	m_dwChangingPhaseTime = ELTimer_GetMSec();
	m_phaseProcessFunc.Set(this, &CPythonNetworkStream::LoadingPhase);
	m_phaseLeaveFunc.Set(this, &CPythonNetworkStream::__LeaveLoadingPhase);

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.Clear();

	CFlyingManager::Instance().DeleteAllInstances();
	CEffectManager::Instance().DeleteAllInstances();

	__DirectEnterMode_Initialize();

	if (m_poHandler && !m_apoPhaseWnd[PHASE_WINDOW_LOAD])
	{
		PyCallClassMemberFunc(m_poHandler, "SetLoadingPhase", Py_BuildValue("()"));
	}
}

bool CPythonNetworkStream::RecvMainCharacter()
{
	TPacketGCMainCharacter pack;
	if (!Recv(sizeof(pack), &pack))
		return false;

	m_dwMainActorVID = pack.dwVID;
	m_dwMainActorRace = pack.wRaceNum;
	m_dwMainActorEmpire = pack.byEmpire;
	m_dwMainActorSkillGroup = pack.bySkillGroup;

	m_rokNetActorMgr->SetMainActorVID(m_dwMainActorVID);

	CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
	rkPlayer.SetName(pack.szName);
	rkPlayer.SetMainCharacterIndex(GetMainActorVID());

	if (pack.szBGMName[0] != '\0')
	{
		if (pack.fBGMVol > 0.0f)
			__SetFieldMusicFileInfo(pack.szBGMName, pack.fBGMVol);
		else
			__SetFieldMusicFileName(pack.szBGMName);
	}

	SendClientVersionPacket();
	__SendInternalBuffer();

	extern bool g_bHeadlessMode;
	if (g_bHeadlessMode)
	{
		Warp(pack.lX, pack.lY);
		CPythonApplication::Instance().SetGlobalCenterPosition(pack.lX, pack.lY);
		SendEnterGame();
		m_isStartGame = TRUE;
		SetGamePhase();
		return true;
	}

	if (m_apoPhaseWnd[PHASE_WINDOW_LOAD])
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOAD], "LoadData", Py_BuildValue("(ii)", pack.lX, pack.lY));
	}
	else if (m_poHandler)
	{
		PyCallClassMemberFunc(m_poHandler, "SetLoadingPhase", Py_BuildValue("()"));
		if (m_apoPhaseWnd[PHASE_WINDOW_LOAD])
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOAD], "LoadData", Py_BuildValue("(ii)", pack.lX, pack.lY));
		}
	}

	__SendInternalBuffer();
	return true;
}

bool CPythonNetworkStream::RecvMainCharacterLuria8A()
{
	uint8_t buffer[77]{};
	if (!Recv(sizeof(buffer), buffer))
		return false;

	uint32_t vid = 0;
	std::memcpy(&vid, buffer + 1, sizeof(uint32_t));

	uint32_t race = 0;
	std::memcpy(&race, buffer + 5, sizeof(uint32_t));

	char name[26]{};
	std::memcpy(name, buffer + 9, 25);
	name[25] = '\0';

	int32_t x = 0, y = 0;
	std::memcpy(&x, buffer + 63, sizeof(int32_t));
	std::memcpy(&y, buffer + 67, sizeof(int32_t));

	uint8_t empire = buffer[72];

	m_dwMainActorVID = vid;
	m_dwMainActorRace = race;
	m_dwMainActorEmpire = empire;
	m_dwMainActorSkillGroup = 0;

	m_rokNetActorMgr->SetMainActorVID(m_dwMainActorVID);

	CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
	rkPlayer.SetName(name);
	rkPlayer.SetMainCharacterIndex(m_dwMainActorVID);

	TraceError("[LURIA_MAIN] Zidentyfikowano postac 0x8A: VID=%u, Name='%s', X=%d, Y=%d, Empire=%u",
		vid, name, x, y, (unsigned int)empire);

	SendClientVersionPacket();
	__SendInternalBuffer();

	extern bool g_bHeadlessMode;
	if (g_bHeadlessMode)
	{
		Warp(x, y);
		CPythonApplication::Instance().SetGlobalCenterPosition(x, y);
		SendEnterGame();
		m_isStartGame = TRUE;
		SetGamePhase();
		return true;
	}

	if (m_apoPhaseWnd[PHASE_WINDOW_LOAD])
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOAD], "LoadData", Py_BuildValue("(ii)", x, y));
	}
	else if (m_poHandler)
	{
		PyCallClassMemberFunc(m_poHandler, "SetLoadingPhase", Py_BuildValue("()"));
		if (m_apoPhaseWnd[PHASE_WINDOW_LOAD])
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOAD], "LoadData", Py_BuildValue("(ii)", x, y));
		}
	}

	__SendInternalBuffer();
	return true;
}

bool CPythonNetworkStream::RecvMainCharacterLuria0F()
{
	uint8_t buffer[47]{};
	if (!Recv(sizeof(buffer), buffer))
		return false;

	uint32_t vid = 0;
	std::memcpy(&vid, buffer + 1, sizeof(uint32_t));

	uint32_t race = 0;
	std::memcpy(&race, buffer + 5, sizeof(uint32_t));

	char name[26]{};
	std::memcpy(name, buffer + 9, 25);
	name[25] = '\0';

	int32_t x = 0, y = 0;
	std::memcpy(&x, buffer + 34, sizeof(int32_t));
	std::memcpy(&y, buffer + 38, sizeof(int32_t));

	uint8_t empire = buffer[42];

	m_dwMainActorVID = vid;
	m_dwMainActorRace = race;
	m_dwMainActorEmpire = empire;

	m_rokNetActorMgr->SetMainActorVID(m_dwMainActorVID);

	CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
	rkPlayer.SetName(name);
	rkPlayer.SetMainCharacterIndex(m_dwMainActorVID);

	TraceError("[LURIA_MAIN] Zidentyfikowano postac 0x0F: VID=%u, Name='%s', X=%d, Y=%d, Empire=%u",
		vid, name, x, y, (unsigned int)empire);

	SendClientVersionPacket();
	__SendInternalBuffer();

	extern bool g_bHeadlessMode;
	if (g_bHeadlessMode)
	{
		Warp(x, y);
		CPythonApplication::Instance().SetGlobalCenterPosition(x, y);
		SendEnterGame();
		m_isStartGame = TRUE;
		SetGamePhase();
		return true;
	}

	if (m_apoPhaseWnd[PHASE_WINDOW_LOAD])
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOAD], "LoadData", Py_BuildValue("(ii)", x, y));
	}
	else if (m_poHandler)
	{
		PyCallClassMemberFunc(m_poHandler, "SetLoadingPhase", Py_BuildValue("()"));
		if (m_apoPhaseWnd[PHASE_WINDOW_LOAD])
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOAD], "LoadData", Py_BuildValue("(ii)", x, y));
		}
	}

	__SendInternalBuffer();
	return true;
}


static std::string gs_fieldMusic_fileName;
static float gs_fieldMusic_volume = 1.0f / 5.0f * 0.1f;

void CPythonNetworkStream::__SetFieldMusicFileName(const char* musicName)
{
	gs_fieldMusic_fileName = musicName;
}

void CPythonNetworkStream::__SetFieldMusicFileInfo(const char* musicName, float vol)
{
	gs_fieldMusic_fileName = musicName;
	gs_fieldMusic_volume = vol;
}

const char* CPythonNetworkStream::GetFieldMusicFileName()
{
	return gs_fieldMusic_fileName.c_str();	
}

float CPythonNetworkStream::GetFieldMusicVolume()
{
	return gs_fieldMusic_volume;
}
// END_OF_SUPPORT_BGM


bool CPythonNetworkStream::__RecvPlayerPoints()
{
	TraceError("[POINTS_DEBUG] ENTERED __RecvPlayerPoints, sizeof=%zu", sizeof(TPacketGCPoints));
	TPacketGCPoints PointsPacket;

	if (!Recv(sizeof(TPacketGCPoints), &PointsPacket))
	{
		TraceError("[POINTS_DEBUG] Recv failed!");
		return false;
	}
	TraceError("[POINTS_DEBUG] Recv succeeded, PointsPacket.header=0x%04X, length=%u", PointsPacket.header, PointsPacket.length);

	for (DWORD i = 0; i < POINT_MAX_NUM; ++i)
	{
		CPythonPlayer::Instance().SetStatus(i, PointsPacket.points[i]);

		if (i == POINT_LEVEL)
			m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].byLevel = PointsPacket.points[i];
		else if (i == POINT_ST)
			m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].byST = PointsPacket.points[i];
		else if (i == POINT_HT)
			m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].byHT = PointsPacket.points[i];
		else if (i == POINT_DX)
			m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].byDX = PointsPacket.points[i];
		else if (i == POINT_IQ)
			m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].byIQ = PointsPacket.points[i];
	}

	TraceError("[POINTS_DEBUG] points set complete, Level=%d, HP=%d", PointsPacket.points[POINT_LEVEL], PointsPacket.points[POINT_HP]);

	__RefreshStatus();
	__RefreshSkillWindow();
	__RefreshInventoryWindow();

	CInstanceBase * pMainInstance = CPythonCharacterManager::Instance().GetMainActorPtr();
	if (pMainInstance)
	{
		pMainInstance->SetMoveSpeed(CPythonPlayer::Instance().GetStatus(POINT_MOV_SPEED));
		pMainInstance->SetAttackSpeed(CPythonPlayer::Instance().GetStatus(POINT_ATT_SPEED));
	}

	if (m_apoPhaseWnd[PHASE_WINDOW_GAME])
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshStatus", Py_BuildValue("()"));
	}
	TraceError("[POINTS_DEBUG] __RecvPlayerPoints EXITING RETURNING TRUE");
	return true;
}

void CPythonNetworkStream::StartGame()
{
	m_isStartGame=TRUE;
}

bool CPythonNetworkStream::SendEnterGame()
{
	if ("Game" == m_strPhase || m_bEnterGameSent)
		return true;

	m_bEnterGameSent = true;

	auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
	if (pDriver)
	{
		auto encodedResult = pDriver->EncodeEnterGame();
		if (encodedResult.has_value() && !encodedResult.value().empty())
		{
			const auto& bytes = encodedResult.value();
			if (!Send(static_cast<int>(bytes.size()), bytes.data()))
				return false;

			__SendInternalBuffer();
			return true;
		}
	}

	TPacketCGEnterFrontGame EnterFrontGamePacket;

	EnterFrontGamePacket.header = CG::ENTERGAME;
	EnterFrontGamePacket.length = sizeof(EnterFrontGamePacket);

	if (!Send(sizeof(EnterFrontGamePacket), &EnterFrontGamePacket))
		return false;

	__SendInternalBuffer();
	return true;
}
