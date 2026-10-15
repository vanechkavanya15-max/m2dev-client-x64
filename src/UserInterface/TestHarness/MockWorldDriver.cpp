#include "StdAfx.h"
#include "MockWorldDriver.h"
#include "../InstanceBase.h"
#include "../PythonCharacterManager.h"
#include "../PythonPlayer.h"
#include "../PythonNetworkStream.h"
#include "../PythonNetworkStreamPhaseGameSync.h"
#include "../AbstractPlayer.h"
#include "EterBase/Timer.h"
#include "GameLib/ActorInstance.h"

namespace UserInterface::TestHarness
{
    bool MockWorldDriver::EnterMockWorld(
        DWORD dwMainVID,
        DWORD dwRace,
        const std::string& name,
        long posX,
        long posY)
    {
        if (m_hasEntered.load(std::memory_order_relaxed))
            return true;

        m_dwMockVID = dwMainVID;

        // 1. Przygotowanie struktury SCreateData dla glownego aktora
        CInstanceBase::SCreateData kCreateData{};
        kCreateData.m_bType = CActorInstance::TYPE_PC;
        kCreateData.m_dwStateFlags = 0;
        kCreateData.m_dwEmpireID = 1;
        kCreateData.m_dwGuildID = 0;
        kCreateData.m_dwLevel = 75;
        kCreateData.m_dwVID = dwMainVID;
        kCreateData.m_dwRace = dwRace;
        kCreateData.m_dwMovSpd = 100;
        kCreateData.m_dwAtkSpd = 100;
        kCreateData.m_lPosX = posX;
        kCreateData.m_lPosY = posY;
        kCreateData.m_fRot = 0.0f;
        kCreateData.m_dwArmor = 0;
        kCreateData.m_dwWeapon = 0;
        kCreateData.m_dwHair = 0;
        kCreateData.m_dwMountVnum = 0;
        kCreateData.m_sAlignment = 0;
        kCreateData.m_byPKMode = 0;
        kCreateData.m_stName = name;
        kCreateData.m_isMain = true;

        // 2. Utworzenie instancji postaci w CPythonCharacterManager
        CPythonCharacterManager& rkChrMgr = CPythonCharacterManager::Instance();
        CInstanceBase* pActor = rkChrMgr.GetInstancePtr(dwMainVID);
        if (!pActor)
        {
            pActor = rkChrMgr.CreateInstance(kCreateData);
        }

        if (pActor)
        {
            rkChrMgr.SetMainInstance(dwMainVID);
            // Rejestracja w podsystemie ActorRegistry
            rkChrMgr.GetSubsystemActorRegistry().RegisterActor(dwMainVID, pActor);
        }

        // 3. Konfiguracja IAbstractPlayer i statystyk w CPythonPlayer
        IAbstractPlayer& rkAbstractPlayer = IAbstractPlayer::GetSingleton();
        rkAbstractPlayer.SetMainCharacterIndex(dwMainVID);

        CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
        rkPlayer.SetStatus(POINT_LEVEL, 75);
        rkPlayer.SetStatus(POINT_HP, 5000);
        rkPlayer.SetStatus(POINT_MAX_HP, 5000);
        rkPlayer.SetStatus(POINT_SP, 1000);
        rkPlayer.SetStatus(POINT_MAX_SP, 1000);
        rkPlayer.SetStatus(POINT_STAMINA, 1000);
        rkPlayer.SetStatus(POINT_MAX_STAMINA, 1000);
        rkPlayer.SetStatus(POINT_GOLD, 100000);
        rkPlayer.SetStatus(POINT_ST, 90);
        rkPlayer.SetStatus(POINT_HT, 90);
        rkPlayer.SetStatus(POINT_DX, 90);
        rkPlayer.SetStatus(POINT_IQ, 90);

        // 4. Przelaczenie fazy sieciowej na PhaseGame bez fizycznego gniazda TCP
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
        rkNetStream.SetMainActorVID(dwMainVID);
        rkNetStream.SetMainActorRace(dwRace);
        rkNetStream.SetMainActorEmpire(1);
        PhaseGameSyncBridge::SetGamePhase(&rkNetStream);

        m_hasEntered.store(true, std::memory_order_relaxed);
        return true;
    }
}
