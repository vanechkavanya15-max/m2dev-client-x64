#pragma once

#include "../../Contracts/IPacketRouter.h"
#include "../../Contracts/IGameEvents.h"
#include "NetCombatRouter.h"
#include "NetItemRouter.h"
#include "NetActorRouter.h"
#include <array>
#include <vector>
#include <memory>
#include <cstdint>
#include <string_view>

namespace UserInterface::Network::Routers
{
    /**
     * @brief Centralny dyspozytor pakietow fazy gry (PhaseGame) w architekturze C++23 x64.
     * 
     * CHARAKTERYSTYKA ARCHITEKTURY:
     * 1. Rozdziela zdeserializowane pakiety fazy gry miedzy wyspecjalizowane routery domenowe.
     * 2. Zapewnia Jump Table O(1) dla naglowkow 8-bitowych oraz bezposredni switch O(1) dla opkodow 16-bitowych.
     * 3. KONTRAKT ZERO-DESYNC: Nigdy nie dotyka strumienia TCP i nie wywoluje Recv().
     * 4. SINGLE-THREADED: Dziala wylacznie w glownym watku gry - zero muteksow i watkow tla.
     */
    class PhaseGamePacketDispatcher
    {
    public:
        PhaseGamePacketDispatcher() noexcept;
        ~PhaseGamePacketDispatcher() = default;

        // Zakaz kopiowania dla zachowania spojnosci dyspozytora
        PhaseGamePacketDispatcher(const PhaseGamePacketDispatcher&) = delete;
        PhaseGamePacketDispatcher& operator=(const PhaseGamePacketDispatcher&) = delete;
        PhaseGamePacketDispatcher(PhaseGamePacketDispatcher&&) noexcept = default;
        PhaseGamePacketDispatcher& operator=(PhaseGamePacketDispatcher&&) noexcept = default;

        // Dostep do globalnej instancji (wzorzec Singleton)
        static PhaseGamePacketDispatcher& Instance() noexcept;

        // Rejestracja i wyrejestrowywanie ogolnych routerow IPacketRouter
        void RegisterRouter(UserInterface::Contracts::IPacketRouter* pRouter);
        void UnregisterRouter(UserInterface::Contracts::IPacketRouter* pRouter);
        void ClearRouters() noexcept;

        // Rejestracja dedykowanych routerow domenowych
        void SetCombatRouter(NetCombatRouter* pRouter) noexcept;
        [[nodiscard]] NetCombatRouter* GetCombatRouter() const noexcept;

        void SetItemRouter(NetItemRouter* pRouter) noexcept;
        [[nodiscard]] NetItemRouter* GetItemRouter() const noexcept;

        void SetActorRouter(NetActorRouter* pRouter) noexcept;
        [[nodiscard]] NetActorRouter* GetActorRouter() const noexcept;

        // Inicjalizacja domyslnego zestawu routerow
        void RegisterDefaultRouters(UserInterface::Contracts::IGameEventSink* pEventSink = nullptr);

        // Zapytania Jump Table O(1)
        [[nodiscard]] UserInterface::Contracts::IPacketRouter* GetRouterForHeader(uint8_t bHeader) const noexcept;
        [[nodiscard]] UserInterface::Contracts::IPacketRouter* GetRouterForHeader(uint16_t wHeader) const noexcept;
        [[nodiscard]] bool HasHandlerForHeader(uint8_t bHeader) const noexcept;
        [[nodiscard]] bool HasHandlerForHeader(uint16_t wHeader) const noexcept;

        // ====================================================================
        // Metody dyspozycji zdeserializowanych pakietow walki (Combat)
        // ====================================================================
        bool DispatchAttack(const TPacketGCAttack& packet);
        bool DispatchDamageInfo(const TPacketGCDamageInfo& packet);
        bool DispatchFly(const TPacketGCCreateFly& packet);
        bool DispatchFlyTargeting(const TPacketGCFlyTargeting& packet);
        bool DispatchDuelStart(const TPacketGCDuelStart& packet);

        // ====================================================================
        // Metody dyspozycji zdeserializowanych pakietow przedmiotow (Item)
        // ====================================================================
        bool DispatchItemSet(const TPacketGCItemSet& packet);
        bool DispatchItemDel(const TPacketGCItemDel& packet);
        bool DispatchItemGroundAdd(const TPacketGCItemGroundAdd& packet);
        bool DispatchItemGroundDel(const TPacketGCItemGroundDel& packet);
        bool DispatchQuickSlotAdd(const TPacketGCQuickSlotAdd& packet);
        bool DispatchQuickSlotDel(const TPacketGCQuickSlotDel& packet);
        bool DispatchQuickSlotSwap(const TPacketGCQuickSlotSwap& packet);

        // ====================================================================
        // Metody dyspozycji zdeserializowanych pakietow aktorow (Actor)
        // ====================================================================
        bool DispatchCharacterAdd(const TPacketGCCharacterAdd& packet);
        bool DispatchCharacterAdditionalInfo(const TPacketGCCharacterAdditionalInfo& packet);
        bool DispatchCharacterDelete(const TPacketGCCharacterDelete& packet);
        bool DispatchObserverMove(const TPacketGCObserverMove& packet);
        bool DispatchSyncPosition(const TPacketGCSyncPosition& packet);

        // Uniwersalna dyspozycja zdeserializowanego rekordu na bazie naglowka (O(1))
        bool DispatchPacket(uint8_t bHeader, const void* pData);
        bool DispatchPacket(uint16_t wHeader, const void* pData);

    private:
        // Tablica skokow O(1) dla naglowkow 8-bitowych (0..255)
        std::array<UserInterface::Contracts::IPacketRouter*, 256> m_jumpTable8{};

        // Wskazniki do wyspecjalizowanych routerow domenowych
        NetCombatRouter* m_pCombatRouter{nullptr};
        NetItemRouter* m_pItemRouter{nullptr};
        NetActorRouter* m_pActorRouter{nullptr};

        // Lista zarejestrowanych routerow
        std::vector<UserInterface::Contracts::IPacketRouter*> m_registeredRouters;

        // Domyslne instancje routerow tworzone w RegisterDefaultRouters
        std::unique_ptr<NetCombatRouter> m_defaultCombatRouter;
        std::unique_ptr<NetItemRouter> m_defaultItemRouter;
        std::unique_ptr<NetActorRouter> m_defaultActorRouter;
    };
}

// Globalny alias w przestrzeni Network::Routers dla wygody
namespace Network::Routers
{
    using PhaseGamePacketDispatcher = UserInterface::Network::Routers::PhaseGamePacketDispatcher;
}
