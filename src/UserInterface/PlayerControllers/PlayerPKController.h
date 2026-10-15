#pragma once

#include "../Core/EngineForwardDecls.h"
#include "../Contracts/IActorProvider.h"
#include "../Contracts/INetworkService.h"
#include <cstdint>
#include <set>

namespace UserInterface::PlayerControllers
{
    /**
     * @brief Kontroler odpowiedzialny za tryby PK, wyzwania, zemste oraz algorytm CanAttack.
     * Zgodny z guardrailami: ZERO MUTEXES, NEVER CACHE POINTERS, INETWORKSERVICE.
     */
    class PlayerPKController
    {
    public:
        PlayerPKController(
            Contracts::IActorProvider* pActorProvider = nullptr,
            Contracts::INetworkService* pNetworkService = nullptr);
        ~PlayerPKController() = default;

        // Rejestracja dostawcow uslug
        void SetActorProvider(Contracts::IActorProvider* pActorProvider) { m_pActorProvider = pActorProvider; }
        void SetNetworkService(Contracts::INetworkService* pNetworkService) { m_pNetworkService = pNetworkService; }

        // Zarzadzanie pamiecia wyzwan / zemsty / niemoznosci walki
        void RememberChallengeInstance(DWORD dwVID);
        void RememberRevengeInstance(DWORD dwVID);
        void RememberCantFightInstance(DWORD dwVID);
        void ForgetInstance(DWORD dwVID);
        void Clear();

        bool IsChallengeInstance(DWORD dwVID) const;
        bool IsRevengeInstance(DWORD dwVID) const;
        bool IsCantFightInstance(DWORD dwVID) const;

        // Glowny algorytm sprawdzania mozliwosci ataku postaci (1:1 z CInstanceBase::IsAttackableInstance)
        bool CanAttack(CInstanceBase* pkInstMain, CInstanceBase* pkInstVictim) const;

    private:
        Contracts::IActorProvider* m_pActorProvider{nullptr};
        Contracts::INetworkService* m_pNetworkService{nullptr};

        std::set<DWORD> m_ChallengeInstanceSet;
        std::set<DWORD> m_RevengeInstanceSet;
        std::set<DWORD> m_CantFightInstanceSet;
    };
}
