#pragma once

#include "../Core/EngineForwardDecls.h"
#include "../Contracts/IActorProvider.h"
#include "../Contracts/INetworkService.h"
#include "../PythonPlayer.h"
#include <cstdint>

namespace UserInterface::PlayerControllers
{
    /**
     * @brief Kontroler odpowiedzialny za kalkulacje bojowe, auto-atak, zasieg broni i combo/emotki.
     * Zgodny z guardrailami: ZERO MUTEXES, NEVER CACHE POINTERS, INETWORKSERVICE.
     */
    class PlayerCombatController
    {
    public:
        PlayerCombatController(
            Contracts::IActorProvider* pActorProvider = nullptr,
            Contracts::INetworkService* pNetworkService = nullptr);
        ~PlayerCombatController() = default;

        // Rejestracja dostawcow uslug
        void SetActorProvider(Contracts::IActorProvider* pActorProvider) { m_pActorProvider = pActorProvider; }
        void SetNetworkService(Contracts::INetworkService* pNetworkService) { m_pNetworkService = pNetworkService; }

        // Konfiguracja mocy broni i rasy
        void SetWeaponPower(DWORD dwMinPower, DWORD dwMaxPower, DWORD dwMinMagicPower, DWORD dwMaxMagicPower, DWORD dwAddPower);
        void SetRace(DWORD dwRace) { m_dwRace = dwRace; }
        DWORD GetRace() const { return m_dwRace; }

        // Statystyki postaci do kalkulacji bojowych
        void SetPlayerStats(DWORD dwLevel, DWORD dwST, DWORD dwDX, DWORD dwIQ, DWORD dwHT);

        // Kalkulacja obrazen (algorytmy 1:1 z CPythonPlayer)
        DWORD GetRaceStat() const;
        DWORD GetLevelAtk() const;
        DWORD GetStatAtk() const;
        DWORD GetWeaponAtk(DWORD dwWeaponPower) const;
        DWORD GetTotalAtk(DWORD dwWeaponPower, DWORD dwRefineBonus) const;
        DWORD GetHitRate() const;
        DWORD GetEvadeRate() const;

        DWORD GetMinAtk() const { return GetTotalAtk(m_dwWeaponMinPower, m_dwWeaponAddPower); }
        DWORD GetMaxAtk() const { return GetTotalAtk(m_dwWeaponMaxPower, m_dwWeaponAddPower); }
        DWORD GetMinWeaponPower() const { return m_dwWeaponMinPower + m_dwWeaponAddPower; }
        DWORD GetMaxWeaponPower() const { return m_dwWeaponMaxPower + m_dwWeaponAddPower; }
        DWORD GetMinMagicWeaponPower() const { return m_dwWeaponMinMagicPower + m_dwWeaponAddPower; }
        DWORD GetMaxMagicWeaponPower() const { return m_dwWeaponMaxMagicPower + m_dwWeaponAddPower; }

        // Auto-atak
        void SetAutoAttackTargetActorID(DWORD dwVID);
        void ClearAutoAttackTargetActorID();
        DWORD GetAutoAttackTargetVID() const { return m_dwAutoAttackTargetVID; }
        void UpdateAutoAttack(float fElapsedTime);

        // Zdolnosc ataku i zasieg
        bool CanAttack() const;
        bool CanShot(DWORD dwTargetVID) const;
        bool IsAttackableDistance(DWORD dwTargetVID) const;
        bool Attack(float fDirRot = -1.0f);

        // Combo i emotki
        void SetComboSkillFlag(BOOL bFlag);
        BOOL GetComboSkillFlag() const { return m_bComboSkillFlag; }
        void SetComboOld(UINT iCombo) { m_iComboOld = iCombo; }
        UINT GetComboOld() const { return m_iComboOld; }

        void StartEmotionProcess() { m_bisProcessingEmotion = TRUE; }
        void EndEmotionProcess() { m_bisProcessingEmotion = FALSE; }
        BOOL IsProcessingEmotion() const { return m_bisProcessingEmotion; }

        // Wysylanie pakietow przez INetworkService
        bool SendAttackPacket(DWORD dwVictimVID, BYTE byType);

    private:
        Contracts::IActorProvider* m_pActorProvider{nullptr};
        Contracts::INetworkService* m_pNetworkService{nullptr};

        // Moce broni
        DWORD m_dwWeaponMinPower{0};
        DWORD m_dwWeaponMaxPower{0};
        DWORD m_dwWeaponMinMagicPower{0};
        DWORD m_dwWeaponMaxMagicPower{0};
        DWORD m_dwWeaponAddPower{0};

        // Parametry postaci
        DWORD m_dwRace{0};
        DWORD m_dwLevel{1};
        DWORD m_dwST{0};
        DWORD m_dwDX{0};
        DWORD m_dwIQ{0};
        DWORD m_dwHT{0};

        // Auto-atak
        DWORD m_dwAutoAttackTargetVID{0};

        // Combo
        UINT m_iComboOld{0};
        BOOL m_bComboSkillFlag{FALSE};

        // Emotki
        BOOL m_bisProcessingEmotion{FALSE};
    };
}
