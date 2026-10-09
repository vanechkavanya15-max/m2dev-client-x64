#pragma once

#include "../Core/EngineForwardDecls.h"
#include "../Contracts/IActorProvider.h"
#include "../Contracts/INetworkService.h"
#include <cstdint>

namespace UserInterface::PlayerControllers
{
    /**
     * @brief Kontroler odpowiedzialny za zbieranie przedmiotow/yang oraz zarzadzanie miksturami automatycznymi (AutoPotion).
     * Zgodny z guardrailami: ZERO MUTEXES, NEVER CACHE POINTERS, INETWORKSERVICE.
     */
    class PlayerItemController
    {
    public:
        enum EAutoPotionType
        {
            AUTO_POTION_TYPE_HP = 0,
            AUTO_POTION_TYPE_SP = 1,
            AUTO_POTION_TYPE_NUM = 2,
        };

        struct SAutoPotionInfo
        {
            bool bActivated{false};
            long currentAmount{0};
            long totalAmount{0};
            long inventorySlotIndex{0};
        };

    public:
        PlayerItemController(
            Contracts::IActorProvider* pActorProvider = nullptr,
            Contracts::INetworkService* pNetworkService = nullptr);
        ~PlayerItemController() = default;

        // Rejestracja dostawcow uslug
        void SetActorProvider(Contracts::IActorProvider* pActorProvider) { m_pActorProvider = pActorProvider; }
        void SetNetworkService(Contracts::INetworkService* pNetworkService) { m_pNetworkService = pNetworkService; }

        // Zarzadzanie AutoPotion
        const SAutoPotionInfo& GetAutoPotionInfo(int type) const;
        SAutoPotionInfo& GetAutoPotionInfo(int type);
        void SetAutoPotionInfo(int type, const SAutoPotionInfo& info);

        // Aktualizacja klatki i petla auto-potion
        void Update(float fElapsedTime);
        void ProcessAutoPotion();

        // Podnoszenie przedmiotow i yang
        void PickCloseItem();
        void PickCloseMoney();
        DWORD GetPickableDistance() const;

        // Wysylanie pakietow przez INetworkService
        bool SendItemPickUpPacket(DWORD dwIID);
        bool SendItemUsePacket(DWORD dwCell);

    private:
        Contracts::IActorProvider* m_pActorProvider{nullptr};
        Contracts::INetworkService* m_pNetworkService{nullptr};

        SAutoPotionInfo m_kAutoPotionInfo[AUTO_POTION_TYPE_NUM];
        float m_fPickupDistance{300.0f};
        DWORD m_dwLastPickItemID{0};
    };
}
