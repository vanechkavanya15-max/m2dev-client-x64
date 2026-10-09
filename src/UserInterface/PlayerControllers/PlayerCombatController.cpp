#include "../StdAfx.h"
#include "PlayerCombatController.h"

#include "../InstanceBase.h"
#include "GameLib/RaceData.h"
#include <algorithm>

namespace UserInterface::PlayerControllers
{
    PlayerCombatController::PlayerCombatController(
        Contracts::IActorProvider* pActorProvider,
        Contracts::INetworkService* pNetworkService)
        : m_pActorProvider(pActorProvider)
        , m_pNetworkService(pNetworkService)
        , m_dwWeaponMinPower(0)
        , m_dwWeaponMaxPower(0)
        , m_dwWeaponMinMagicPower(0)
        , m_dwWeaponMaxMagicPower(0)
        , m_dwWeaponAddPower(0)
        , m_dwRace(0)
        , m_dwLevel(1)
        , m_dwST(0)
        , m_dwDX(0)
        , m_dwIQ(0)
        , m_dwHT(0)
        , m_dwAutoAttackTargetVID(0)
        , m_iComboOld(0)
        , m_bComboSkillFlag(FALSE)
        , m_bisProcessingEmotion(FALSE)
    {
    }

    void PlayerCombatController::SetWeaponPower(
        DWORD dwMinPower,
        DWORD dwMaxPower,
        DWORD dwMinMagicPower,
        DWORD dwMaxMagicPower,
        DWORD dwAddPower)
    {
        m_dwWeaponMinPower = dwMinPower;
        m_dwWeaponMaxPower = dwMaxPower;
        m_dwWeaponMinMagicPower = dwMinMagicPower;
        m_dwWeaponMaxMagicPower = dwMaxMagicPower;
        m_dwWeaponAddPower = dwAddPower;
    }

    void PlayerCombatController::SetPlayerStats(DWORD dwLevel, DWORD dwST, DWORD dwDX, DWORD dwIQ, DWORD dwHT)
    {
        m_dwLevel = dwLevel;
        m_dwST = dwST;
        m_dwDX = dwDX;
        m_dwIQ = dwIQ;
        m_dwHT = dwHT;
    }

    DWORD PlayerCombatController::GetRaceStat() const
    {
        switch (m_dwRace)
        {
            case MAIN_RACE_WARRIOR_M:
            case MAIN_RACE_WARRIOR_W:
                return m_dwST;

            case MAIN_RACE_ASSASSIN_M:
            case MAIN_RACE_ASSASSIN_W:
                return m_dwDX;

            case MAIN_RACE_SURA_M:
            case MAIN_RACE_SURA_W:
                return m_dwST;

            case MAIN_RACE_SHAMAN_M:
            case MAIN_RACE_SHAMAN_W:
                return m_dwIQ;

            default:
                break;
        }

        return m_dwST;
    }

    DWORD PlayerCombatController::GetLevelAtk() const
    {
        return 2 * m_dwLevel;
    }

    DWORD PlayerCombatController::GetStatAtk() const
    {
        return (4 * m_dwST + 2 * GetRaceStat()) / 3;
    }

    DWORD PlayerCombatController::GetWeaponAtk(DWORD dwWeaponPower) const
    {
        return 2 * dwWeaponPower;
    }

    DWORD PlayerCombatController::GetHitRate() const
    {
        int src = static_cast<int>((m_dwDX * 4 + m_dwLevel * 2) / 6);
        return static_cast<DWORD>(100 * (std::min(90, src) + 210) / 300);
    }

    DWORD PlayerCombatController::GetEvadeRate() const
    {
        if (m_dwDX + 95 == 0)
            return 0;

        return static_cast<DWORD>(30 * (2 * m_dwDX + 5) / (m_dwDX + 95));
    }

    DWORD PlayerCombatController::GetTotalAtk(DWORD dwWeaponPower, DWORD dwRefineBonus) const
    {
        DWORD dwLvAtk = GetLevelAtk();
        DWORD dwStAtk = GetStatAtk();
        int hr = static_cast<int>(GetHitRate());
        DWORD dwWepAtk = GetWeaponAtk(dwWeaponPower + dwRefineBonus);
        DWORD dwTotalAtk = dwLvAtk + (dwStAtk + dwWepAtk) * hr / 100;

        return dwTotalAtk;
    }

    void PlayerCombatController::SetAutoAttackTargetActorID(DWORD dwVID)
    {
        m_dwAutoAttackTargetVID = dwVID;
    }

    void PlayerCombatController::ClearAutoAttackTargetActorID()
    {
        m_dwAutoAttackTargetVID = 0;
    }

    void PlayerCombatController::UpdateAutoAttack(float fElapsedTime)
    {
        if (0 == m_dwAutoAttackTargetVID)
            return;

        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return;

        CInstanceBase* pkInstVictim = m_pActorProvider ? m_pActorProvider->GetInstance(m_dwAutoAttackTargetVID) : nullptr;
        if (!pkInstVictim)
        {
            ClearAutoAttackTargetActorID();
            return;
        }

        if (pkInstVictim->IsDead())
        {
            ClearAutoAttackTargetActorID();
            return;
        }

        if (pkInstMain->IsMountingHorse() && !pkInstMain->CanAttackHorseLevel())
        {
            ClearAutoAttackTargetActorID();
            return;
        }

        if (pkInstMain->IsAttackableInstance(*pkInstVictim))
        {
            if (pkInstMain->IsSleep())
            {
                return;
            }

            if (pkInstMain->NEW_IsClickableDistanceDestInstance(*pkInstVictim))
            {
                pkInstMain->NEW_AttackToDestInstanceDirection(*pkInstVictim);
                SendAttackPacket(m_dwAutoAttackTargetVID, 0);
            }
            else
            {
                pkInstMain->NEW_MoveToDestInstanceDirection(*pkInstVictim);
            }
        }
    }

    bool PlayerCombatController::CanAttack() const
    {
        if (m_bisProcessingEmotion)
            return false;

        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return false;

        if (pkInstMain->IsMountingHorse() && pkInstMain->IsNewMount())
        {
            // Nowy mount wymaga uprawnien
        }

        return pkInstMain->CanAttack();
    }

    bool PlayerCombatController::CanShot(DWORD dwTargetVID) const
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return false;

        CInstanceBase* pkInstTarget = m_pActorProvider ? m_pActorProvider->GetInstance(dwTargetVID) : nullptr;
        if (!pkInstTarget)
            return false;

        if (pkInstMain->IsInSafe() || pkInstTarget->IsInSafe())
            return false;

        return true;
    }

    bool PlayerCombatController::IsAttackableDistance(DWORD dwTargetVID) const
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return false;

        CInstanceBase* pkInstTarget = m_pActorProvider ? m_pActorProvider->GetInstance(dwTargetVID) : nullptr;
        if (!pkInstTarget)
            return false;

        return pkInstMain->NEW_IsClickableDistanceDestInstance(*pkInstTarget);
    }

    bool PlayerCombatController::Attack(float fDirRot)
    {
        if (!CanAttack())
            return false;

        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return false;

        if (fDirRot >= 0.0f)
            pkInstMain->NEW_Attack(fDirRot);
        else
            pkInstMain->NEW_Attack();

        return true;
    }

    void PlayerCombatController::SetComboSkillFlag(BOOL bFlag)
    {
        m_bComboSkillFlag = bFlag;

        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (pkInstMain)
        {
            pkInstMain->SetComboType(bFlag ? 1 : 0);
        }
    }

    bool PlayerCombatController::SendAttackPacket(DWORD dwVictimVID, BYTE byType)
    {
        if (m_pNetworkService)
            return m_pNetworkService->SendAttackPacket(dwVictimVID, byType);

        return false;
    }
}
