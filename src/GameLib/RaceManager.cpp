#include "StdAfx.h"
#include "RaceManager.h"
#include "RaceMotionData.h"
#include "PackLib/PackManager.h"
#include "EterLib/GameThreadPool.h"
#include <future>
#include <vector>
#include <set>
#include <algorithm>

bool __IsGuildRace(unsigned race)
{
	if (race >= 14000 && race < 15000)
		return true;

	if (20043 == race)
		return true;

	return false;
}

bool __IsNPCRace(unsigned race)
{
	if (race > 9000)
		return true;

	return false;
}

void __GetRaceResourcePathes(unsigned race, std::vector <std::string>& vec_stPathes)
{
	if (__IsGuildRace(race))
	{
		vec_stPathes.push_back ("d:/ymir work/guild/");
		vec_stPathes.push_back ("d:/ymir work/npc/");
		vec_stPathes.push_back ("d:/ymir work/npc2/");
		vec_stPathes.push_back ("d:/ymir work/monster/");
		vec_stPathes.push_back ("d:/ymir work/monster2/");
	}
	else if (__IsNPCRace(race))
	{
		if (race >= 30000)
		{
			vec_stPathes.push_back ("d:/ymir work/npc2/");
			vec_stPathes.push_back ("d:/ymir work/npc/");
			vec_stPathes.push_back ("d:/ymir work/monster/");
			vec_stPathes.push_back ("d:/ymir work/monster2/");
			vec_stPathes.push_back ("d:/ymir work/guild/");
		}
		else
		{
			vec_stPathes.push_back ("d:/ymir work/npc/");
			vec_stPathes.push_back ("d:/ymir work/npc2/");
			vec_stPathes.push_back ("d:/ymir work/monster/");
			vec_stPathes.push_back ("d:/ymir work/monster2/");
			vec_stPathes.push_back ("d:/ymir work/guild/");
		}
	}
	// 만우절 이벤트용 예외 몬스터
	else if (8507 == race || 8510 == race)
	{
		vec_stPathes.push_back ("d:/ymir work/monster2/");
		vec_stPathes.push_back ("d:/ymir work/monster/");
		vec_stPathes.push_back ("d:/ymir work/npc/");
		vec_stPathes.push_back ("d:/ymir work/npc2/");
		vec_stPathes.push_back ("d:/ymir work/guild/");
	}
	else if (race > 8000)
	{
		vec_stPathes.push_back ("d:/ymir work/monster/");
		vec_stPathes.push_back ("d:/ymir work/monster2/");
		vec_stPathes.push_back ("d:/ymir work/npc/");
		vec_stPathes.push_back ("d:/ymir work/npc2/");
		vec_stPathes.push_back ("d:/ymir work/guild/");
	}
	else if (race > 2000)
	{
		vec_stPathes.push_back ("d:/ymir work/monster2/");
		vec_stPathes.push_back ("d:/ymir work/monster/");
		vec_stPathes.push_back ("d:/ymir work/npc/");
		vec_stPathes.push_back ("d:/ymir work/npc2/");
		vec_stPathes.push_back ("d:/ymir work/guild/");
	}
	else if (race>=1400 && race<=1700)
	{
		vec_stPathes.push_back ("d:/ymir work/monster2/");
		vec_stPathes.push_back ("d:/ymir work/monster/");
		vec_stPathes.push_back ("d:/ymir work/npc/");
		vec_stPathes.push_back ("d:/ymir work/npc2/");
		vec_stPathes.push_back ("d:/ymir work/guild/");
	}
	else
	{
		vec_stPathes.push_back ("d:/ymir work/monster/");
		vec_stPathes.push_back ("d:/ymir work/monster2/");
		vec_stPathes.push_back ("d:/ymir work/npc/");
		vec_stPathes.push_back ("d:/ymir work/npc2/");
		vec_stPathes.push_back ("d:/ymir work/guild/");
	}
	return;
}

CRaceData* CRaceManager::__LoadRaceData(DWORD dwRaceIndex)
{
	std::map<DWORD, std::string>::iterator fRaceName=m_kMap_dwRaceKey_stRaceName.find(dwRaceIndex);
	if (m_kMap_dwRaceKey_stRaceName.end()==fRaceName)
		return NULL;

	const std::string& c_rstRaceName=fRaceName->second;

	if (c_rstRaceName.empty())
		return NULL;
	
	// LOAD_LOCAL_RESOURCE
	if (c_rstRaceName[0] == '#')
	{	
		const char* pathName = c_rstRaceName.c_str() + 1;
		char shapeFileName[256];
		char motionListFileName[256];
		_snprintf(shapeFileName, sizeof(shapeFileName), "%sshape.msm", pathName);
		_snprintf(motionListFileName, sizeof(motionListFileName), "%smotlist.txt", pathName);
				
		CRaceData * pRaceData = CRaceData::New();
		pRaceData->SetRace(dwRaceIndex);
		if (!pRaceData->LoadRaceData(shapeFileName))
		{
			TraceError("CRaceManager::RegisterRacePath(race=%u).LoadRaceData(%s)", dwRaceIndex, shapeFileName);
			CRaceData::Delete(pRaceData);
			return NULL;
		}

		__LoadRaceMotionList(*pRaceData, pathName, motionListFileName);		

		return pRaceData;
	}
	// END_OF_LOAD_LOCAL_RESOURCE
	std::vector <std::string> vec_stFullPathName;
	__GetRaceResourcePathes(dwRaceIndex, vec_stFullPathName);

	CRaceData * pRaceData = CRaceData::New();
	pRaceData->SetRace(dwRaceIndex);
	
	for (int i = 0; i < vec_stFullPathName.size(); i++)
	{
		std::string stFullPathName = vec_stFullPathName[i];
		{
			std::map<std::string, std::string>::iterator fRaceSrcName=m_kMap_stRaceName_stSrcName.find(c_rstRaceName);
			if (m_kMap_stRaceName_stSrcName.end()==fRaceSrcName)
				stFullPathName+=c_rstRaceName;
			else
				stFullPathName+=fRaceSrcName->second;
		}

		stFullPathName+="/";

		std::string stMSMFileName=stFullPathName;
		stMSMFileName+=c_rstRaceName;
		stMSMFileName+=".msm";

		if (!pRaceData->LoadRaceData(stMSMFileName.c_str()))
		{
			if (i != vec_stFullPathName.size() - 1)
			{
				TraceError("CRaceManager::RegisterRacePath : RACE[%u] LOAD MSMFILE[%s] ERROR. Will Find Another Path.", dwRaceIndex, stMSMFileName.c_str());
				continue;
			}
			
			TraceError("CRaceManager::RegisterRacePath : RACE[%u] LOAD MSMFILE[%s] ERROR", dwRaceIndex, stMSMFileName.c_str());
			CRaceData::Delete(pRaceData);
			return NULL;
		}

		std::string stMotionListFileName=stFullPathName;
		stMotionListFileName+=pRaceData->GetMotionListFileName();

		__LoadRaceMotionList(*pRaceData, stFullPathName.c_str(), stMotionListFileName.c_str());		

		return pRaceData;
	}
	TraceError("CRaceManager::RegisterRacePath : RACE[%u] HAVE NO PATH ERROR", dwRaceIndex);
	CRaceData::Delete(pRaceData);
	return NULL;
}

bool CRaceManager::__LoadRaceMotionList(CRaceData& rkRaceData, const char* pathName, const char* motionListFileName)
{
	static std::map<std::string, DWORD> s_kMap_stType_dwIndex;
	static bool s_isInit=false;

	if (!s_isInit)
	{
		s_isInit=true;

		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("SPAWN", CRaceMotionData::NAME_SPAWN));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("WAIT", CRaceMotionData::NAME_WAIT));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("WAIT1", CRaceMotionData::NAME_WAIT));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("WAIT2", CRaceMotionData::NAME_WAIT));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("WALK", CRaceMotionData::NAME_WALK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("WALK1", CRaceMotionData::NAME_WALK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("WALK2", CRaceMotionData::NAME_WALK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("RUN", CRaceMotionData::NAME_RUN));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("RUN1", CRaceMotionData::NAME_RUN));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("RUN2", CRaceMotionData::NAME_RUN));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("STOP", CRaceMotionData::NAME_STOP));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("DEAD", CRaceMotionData::NAME_DEAD));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("COMBO_ATTACK", CRaceMotionData::NAME_COMBO_ATTACK_1));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("COMBO_ATTACK1", CRaceMotionData::NAME_COMBO_ATTACK_2));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("COMBO_ATTACK2", CRaceMotionData::NAME_COMBO_ATTACK_3));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("NORMAL_ATTACK", CRaceMotionData::NAME_NORMAL_ATTACK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("NORMAL_ATTACK1", CRaceMotionData::NAME_NORMAL_ATTACK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("NORMAL_ATTACK2", CRaceMotionData::NAME_NORMAL_ATTACK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("FRONT_DAMAGE", CRaceMotionData::NAME_DAMAGE));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("FRONT_DAMAGE1", CRaceMotionData::NAME_DAMAGE));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("FRONT_DAMAGE2", CRaceMotionData::NAME_DAMAGE));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("FRONT_DAMAGE3", CRaceMotionData::NAME_DAMAGE));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("FRONT_DEAD", CRaceMotionData::NAME_DEAD));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("FRONT_DEAD1", CRaceMotionData::NAME_DEAD));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("FRONT_DEAD2", CRaceMotionData::NAME_DEAD));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("FRONT_KNOCKDOWN", CRaceMotionData::NAME_DAMAGE_FLYING));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("FRONT_KNOCKDOWN1", CRaceMotionData::NAME_DAMAGE_FLYING));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("FRONT_STANDUP", CRaceMotionData::NAME_STAND_UP));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("FRONT_STANDUP1", CRaceMotionData::NAME_STAND_UP));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("BACK_DAMAGE", CRaceMotionData::NAME_DAMAGE_BACK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("BACK_DAMAGE1", CRaceMotionData::NAME_DAMAGE_BACK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("BACK_DEAD", CRaceMotionData::NAME_DEAD_BACK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("BACK_DEAD1", CRaceMotionData::NAME_DEAD_BACK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("BACK_DEAD2", CRaceMotionData::NAME_DEAD_BACK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("BACK_KNOCKDOWN", CRaceMotionData::NAME_DAMAGE_FLYING_BACK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("BACK_KNOCKDOWN1", CRaceMotionData::NAME_DAMAGE_FLYING_BACK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("BACK_STANDUP", CRaceMotionData::NAME_STAND_UP_BACK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("BACK_STANDUP1", CRaceMotionData::NAME_STAND_UP_BACK));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("SPECIAL", CRaceMotionData::NAME_SPECIAL_1));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("SPECIAL1", CRaceMotionData::NAME_SPECIAL_2));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("SPECIAL2", CRaceMotionData::NAME_SPECIAL_3));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("SPECIAL3", CRaceMotionData::NAME_SPECIAL_4));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("SPECIAL4", CRaceMotionData::NAME_SPECIAL_5));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("SPECIAL5", CRaceMotionData::NAME_SPECIAL_6));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("SKILL1", CRaceMotionData::NAME_SKILL+121));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("SKILL2", CRaceMotionData::NAME_SKILL+122));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("SKILL3", CRaceMotionData::NAME_SKILL+123));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("SKILL4", CRaceMotionData::NAME_SKILL+124));
		s_kMap_stType_dwIndex.insert(std::map<std::string, DWORD>::value_type("SKILL5", CRaceMotionData::NAME_SKILL+125));
	}
	
	TPackFile kMappedFile;
	if (!CPackManager::Instance().GetFile(motionListFileName, kMappedFile))
		return false;
	

	CMemoryTextFileLoader kTextFileLoader;
	kTextFileLoader.Bind(kMappedFile.size(), kMappedFile.data());

	rkRaceData.RegisterMotionMode(CRaceMotionData::MODE_GENERAL);

	char szMode[256];
	char szType[256];
	char szFile[256];
	int nPercent = 0;

	bool isSpawn=false;

	static std::string stSpawnMotionFileName;
	static std::string stMotionFileName;

	stSpawnMotionFileName = "";
	stMotionFileName = "";

	UINT uLineCount=kTextFileLoader.GetLineCount();
	for (UINT uLineIndex=0; uLineIndex<uLineCount; ++uLineIndex)
	{
		DWORD motionType = CRaceMotionData::NAME_NONE;

		const std::string& c_rstLine=kTextFileLoader.GetLineString(uLineIndex);
		sscanf(c_rstLine.c_str(), "%s %s %s %d", szMode, szType, szFile, &nPercent);

		std::map<std::string, DWORD>::iterator fTypeIndex=s_kMap_stType_dwIndex.find(szType);

		if (s_kMap_stType_dwIndex.end() == fTypeIndex)
		{
			// 모션 목록에 WAIT, WAIT4, WAIT20  이런 식으로 등록되어 있을 때,
			// WAIT4, WAIT20을 WAIT로 인식할 수 있도록 처리
			const size_t c_cutLengthLimit = 2;
			bool bFound = false;

			if (c_cutLengthLimit < strlen(szType) + 1)
			{
				for (size_t i = 1; i <= c_cutLengthLimit; ++i)
				{
					std::string typeName = std::string(szType).substr(0, strlen(szType) - i);
					fTypeIndex = s_kMap_stType_dwIndex.find(typeName);

					if (s_kMap_stType_dwIndex.end() != fTypeIndex)
					{
						bFound = true;
						break;
					}
				}
			}

			if (false == bFound)
				continue;
		}

		motionType = fTypeIndex->second;

		stMotionFileName = pathName;
		stMotionFileName += szFile; 

		rkRaceData.RegisterMotionData(CRaceMotionData::MODE_GENERAL, motionType, stMotionFileName.c_str(), nPercent);

		switch (motionType)
		{
			case CRaceMotionData::NAME_SPAWN:
				isSpawn=true;
				break;
			case CRaceMotionData::NAME_DAMAGE:
				stSpawnMotionFileName=stMotionFileName;
				break;
		}
	}

	if (!isSpawn && stSpawnMotionFileName!="")
	{
		rkRaceData.RegisterMotionData(CRaceMotionData::MODE_GENERAL, CRaceMotionData::NAME_SPAWN, stSpawnMotionFileName.c_str(), nPercent);
	}

	rkRaceData.RegisterNormalAttack(CRaceMotionData::MODE_GENERAL, CRaceMotionData::NAME_NORMAL_ATTACK);

	return true;
}

void CRaceManager::RegisterRaceSrcName(const char * c_szName, const char * c_szSrcName)
{
	m_kMap_stRaceName_stSrcName.insert(std::map<std::string, std::string>::value_type(c_szName, c_szSrcName));
}

void CRaceManager::RegisterRaceName(DWORD dwRaceIndex, const char * c_szName)
{	
	m_kMap_dwRaceKey_stRaceName.insert(std::map<DWORD, std::string>::value_type(dwRaceIndex, c_szName));
}

void CRaceManager::CreateRace(DWORD dwRaceIndex)
{
	std::lock_guard<std::mutex> lock(m_RaceDataMapMutex);
	if (m_RaceDataMap.end() != m_RaceDataMap.find(dwRaceIndex))
	{
		TraceError("RaceManager::CreateRace : Race %u already created", dwRaceIndex);
		return;
	}

	CRaceData * pRaceData = CRaceData::New();
	pRaceData->SetRace(dwRaceIndex);
	m_RaceDataMap.insert(TRaceDataMap::value_type(dwRaceIndex, pRaceData));

	Tracenf("CRaceManager::CreateRace(dwRaceIndex=%d)", dwRaceIndex);
}

void CRaceManager::SelectRace(DWORD dwRaceIndex)
{
	std::lock_guard<std::mutex> lock(m_RaceDataMapMutex);
	TRaceDataIterator itor = m_RaceDataMap.find(dwRaceIndex);
	if (m_RaceDataMap.end() == itor)
	{
		assert(!"Failed to select race data!");
		return;
	}

	m_pSelectedRaceData = itor->second;
}

CRaceData * CRaceManager::GetSelectedRaceDataPointer()
{
	return m_pSelectedRaceData;
}

BOOL CRaceManager::LoadRaceData(const char * c_szFileName)
{
	if (!m_pSelectedRaceData)
		return FALSE;

	const char * c_szFullFileName = GetFullPathFileName(c_szFileName);
	return m_pSelectedRaceData->LoadRaceData(c_szFullFileName);
}

BOOL CRaceManager::LoadLocalRaceData(const char * c_szFileName)
{
	if (!m_pSelectedRaceData)
		return FALSE;

	return m_pSelectedRaceData->LoadRaceData(c_szFileName);
}

BOOL CRaceManager::GetRaceDataPointer(DWORD dwRaceIndex, CRaceData ** ppRaceData)
{
	// Thread-safe lookup
	{
		std::lock_guard<std::mutex> lock(m_RaceDataMapMutex);
		TRaceDataIterator itor = m_RaceDataMap.find(dwRaceIndex);

		if (m_RaceDataMap.end() != itor)
		{
			*ppRaceData = itor->second;
			return TRUE;
		}
	}

	// Check if already loading asynchronously
	{
		std::lock_guard<std::mutex> lock(m_LoadingRacesMutex);
		if (m_LoadingRaces.find(dwRaceIndex) != m_LoadingRaces.end())
		{
			// Race is being loaded asynchronously
			// Wait for it to complete by loading synchronously now (needed for immediate visibility)
			Tracef("CRaceManager::GetRaceDataPointer: Race %lu is loading async, switching to sync load\n", dwRaceIndex);
		}
	}

	// Not found - load synchronously to ensure immediate availability
	CRaceData* pRaceData = __LoadRaceData(dwRaceIndex);

	if (pRaceData)
	{
		std::lock_guard<std::mutex> lock(m_RaceDataMapMutex);
		// Check again in case another thread loaded it
		TRaceDataIterator itor = m_RaceDataMap.find(dwRaceIndex);
		if (m_RaceDataMap.end() != itor)
		{
			// Already loaded by another thread, use that one
			delete pRaceData;
			*ppRaceData = itor->second;
		}
		else
		{
			// Insert our newly loaded data
			m_RaceDataMap.insert(TRaceDataMap::value_type(dwRaceIndex, pRaceData));
			*ppRaceData = pRaceData;
		}

		// Remove from loading set if present
		{
			std::lock_guard<std::mutex> lock2(m_LoadingRacesMutex);
			m_LoadingRaces.erase(dwRaceIndex);
		}

		return TRUE;
	}

	*ppRaceData = NULL;
	return FALSE;
}

void CRaceManager::SetPathName(const char * c_szPathName)
{
	m_strPathName = c_szPathName;
}

const char * CRaceManager::GetFullPathFileName(const char * c_szFileName)
{
	static std::string s_stFileName;
	
	if (c_szFileName[0] != '.')
	{
		s_stFileName = m_strPathName;
		s_stFileName += c_szFileName;
	}
	else
	{
		s_stFileName = c_szFileName;
	}

	return s_stFileName.c_str();
}

void CRaceManager::PreloadCoreRaces()
{
	if (m_bPreloaded)
		return;

	m_bPreloaded = true;

	struct SCoreRaceInfo
	{
		DWORD dwRace;
		const char* szMsm;
		const char* szPath;
		const char* szRaceName;
	};

	static const SCoreRaceInfo s_kCoreRaces[] = {
		{ 0, "msm/warrior_m.msm",  "d:/ymir work/pc/warrior/", "warrior_m" },
		{ 4, "msm/warrior_w.msm",  "d:/ymir work/pc/warrior/", "warrior_w" },
		{ 1, "msm/assassin_w.msm", "d:/ymir work/pc/assassin/", "assassin_w" },
		{ 5, "msm/assassin_m.msm", "d:/ymir work/pc/assassin/", "assassin_m" },
		{ 2, "msm/sura_m.msm",     "d:/ymir work/pc/sura/", "sura_m" },
		{ 6, "msm/sura_w.msm",     "d:/ymir work/pc/sura/", "sura_w" },
		{ 3, "msm/shaman_w.msm",   "d:/ymir work/pc/shaman/", "shaman_w" },
		{ 7, "msm/shaman_m.msm",   "d:/ymir work/pc/shaman/", "shaman_m" },
	};

	for (const auto& race : s_kCoreRaces)
	{
		RegisterRaceName(race.dwRace, race.szRaceName);
		SetPathName(race.szPath);

		// Idempotentne tworzenie i wybieranie rasy
		if (m_RaceDataMap.find(race.dwRace) == m_RaceDataMap.end())
		{
			CreateRace(race.dwRace);
		}
		SelectRace(race.dwRace);

		CRaceData* pRaceData = GetSelectedRaceDataPointer();
		if (pRaceData)
		{
			// 1. Proba bezposredniego zaladowania z relatywnej sciezki MSM
			bool bLoaded = pRaceData->LoadRaceData(race.szMsm);

			// 2. Fallback: root/ oraz assets/root/
			if (!bLoaded)
			{
				std::string strRootMsm = std::string("root/") + race.szMsm;
				bLoaded = pRaceData->LoadRaceData(strRootMsm.c_str());
			}
			if (!bLoaded)
			{
				std::string strAssetsRootMsm = std::string("assets/root/") + race.szMsm;
				bLoaded = pRaceData->LoadRaceData(strAssetsRootMsm.c_str());
			}

			// 3. Fallback: sciezka z pelnym katalogiem rasy
			if (!bLoaded)
			{
				std::string strFullPath = std::string(race.szPath) + race.szMsm;
				bLoaded = pRaceData->LoadRaceData(strFullPath.c_str());
			}

			// 4. Fallback: bez prefiksu msm/
			if (!bLoaded)
			{
				const char* szBaseMsm = strrchr(race.szMsm, '/');
				szBaseMsm = szBaseMsm ? (szBaseMsm + 1) : race.szMsm;
				bLoaded = pRaceData->LoadRaceData(szBaseMsm);

				// 5. Fallback: sciezka z bazowym katalogiem i bez prefiksu msm/
				if (!bLoaded)
				{
					std::string strFullBasePath = std::string(race.szPath) + szBaseMsm;
					bLoaded = pRaceData->LoadRaceData(strFullBasePath.c_str());
				}
			}

			if (!bLoaded)
			{
				TraceError("CRaceManager::PreloadCoreRaces: Failed to load race data for race %u (%s)", race.dwRace, race.szMsm);
			}
		}
	}

	// Wczytaj liste NPC
	LoadNPCList("npclist.txt");
}

bool CRaceManager::LoadNPCList(const char* c_szFileName)
{
	if (!c_szFileName || !*c_szFileName)
		return false;

	TPackFile kFile;
	std::vector<char> diskBuffer;
	const char* pData = nullptr;
	size_t nDataSize = 0;

	std::string strRootFallback = std::string("root/") + c_szFileName;
	std::string strAssetsRootFallback = std::string("assets/root/") + c_szFileName;

	if (CPackManager::Instance().GetFile(c_szFileName, kFile))
	{
		pData = reinterpret_cast<const char*>(kFile.data());
		nDataSize = kFile.size();
	}
	else if (CPackManager::Instance().GetFile(strRootFallback.c_str(), kFile))
	{
		pData = reinterpret_cast<const char*>(kFile.data());
		nDataSize = kFile.size();
	}
	else if (CPackManager::Instance().GetFile(strAssetsRootFallback.c_str(), kFile))
	{
		pData = reinterpret_cast<const char*>(kFile.data());
		nDataSize = kFile.size();
	}
	else
	{
		FILE* fp = fopen(c_szFileName, "rb");
		if (!fp)
		{
			fp = fopen(strRootFallback.c_str(), "rb");
		}
		if (!fp)
		{
			fp = fopen(strAssetsRootFallback.c_str(), "rb");
		}

		if (fp)
		{
			fseek(fp, 0, SEEK_END);
			long fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			if (fileSize > 0)
			{
				diskBuffer.resize(fileSize);
				size_t readBytes = fread(diskBuffer.data(), 1, fileSize, fp);
				diskBuffer.resize(readBytes);
				pData = diskBuffer.data();
				nDataSize = diskBuffer.size();
			}
			fclose(fp);
		}
	}

	if (!pData || nDataSize == 0)
	{
		TraceError("CRaceManager::LoadNPCList: cannot load file %s", c_szFileName);
		return false;
	}

	CMemoryTextFileLoader kTextFileLoader;
	kTextFileLoader.Bind(static_cast<int>(nDataSize), pData);

	CTokenVector kTokenVector;
	DWORD dwLineCount = kTextFileLoader.GetLineCount();

	for (DWORD i = 0; i < dwLineCount; ++i)
	{
		if (!kTextFileLoader.SplitLine(i, &kTokenVector, " \t"))
			continue;

		if (kTokenVector.empty())
			continue;

		const std::string& strVnum = kTokenVector[0];
		if (strVnum.empty())
			continue;

		// Ignoruj komentarze
		if (strVnum[0] == '#' || (strVnum.size() >= 2 && strVnum[0] == '/' && strVnum[1] == '/'))
			continue;

		char* pEnd = nullptr;
		unsigned long dwVnum = strtoul(strVnum.c_str(), &pEnd, 10);
		if (pEnd == strVnum.c_str())
			continue;

		if (dwVnum != 0)
		{
			if (kTokenVector.size() >= 2)
			{
				RegisterRaceName(dwVnum, kTokenVector[1].c_str());
			}
			if (kTokenVector.size() >= 3)
			{
				RegisterRaceSrcName(kTokenVector[1].c_str(), kTokenVector[2].c_str());
			}
		}
		else
		{
			if (kTokenVector.size() >= 3)
			{
				RegisterRaceSrcName(kTokenVector[1].c_str(), kTokenVector[2].c_str());
			}
			else if (kTokenVector.size() >= 2)
			{
				RegisterRaceName(0, kTokenVector[1].c_str());
			}
		}
	}

	return true;
}

void CRaceManager::Create()
{
	CRaceMotionData::CreateSystem(2048);
	CRaceData::CreateSystem(256, 512);

	PreloadCoreRaces();
}

void CRaceManager::__Initialize()
{
	m_pSelectedRaceData = NULL;
	m_bPreloaded = false;
}

void CRaceManager::__DestroyRaceDataMap()
{
	TRaceDataMap::iterator i;
	for (i=m_RaceDataMap.begin(); i!=m_RaceDataMap.end(); ++i)
		CRaceData::Delete(i->second);

	m_RaceDataMap.clear();
}

void CRaceManager::Destroy()
{
	__DestroyRaceDataMap();

	__Initialize();
}

CRaceManager::CRaceManager()
{
	__Initialize();	
}

CRaceManager::~CRaceManager()
{
	Destroy();
}
