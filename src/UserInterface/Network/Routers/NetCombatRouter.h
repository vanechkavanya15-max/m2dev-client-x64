#pragma once

#include "../../Contracts/IPacketRouter.h"
#include "../../Contracts/IGameEvents.h"
#include "Client/Network/Protocol/Protocol.h"
#include "../../Core/EventBus.h"
#include <string_view>
#include <cstdint>

namespace UserInterface::Network::Routers
{
    // Alias zgodny ze specyfikacja pakietow lotu
    using TPacketGCFly = TPacketGCCreateFly;

    // Struktury zdarzen domenowych dla szyny EventBus
    struct CombatAttackDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwAttackerVID;
        uint32_t dwVictimVID;
        uint8_t byMotionType;

        CombatAttackDomainEvent(uint32_t attacker, uint32_t victim, uint8_t motion)
            : dwAttackerVID(attacker), dwVictimVID(victim), byMotionType(motion) {}
    };

    struct CombatDamageInfoDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwVictimVID;
        uint32_t dwAttackerVID;
        int32_t lDamage;
        uint8_t byDamageFlag;

        CombatDamageInfoDomainEvent(uint32_t victim, uint32_t attacker, int32_t damage, uint8_t flag)
            : dwVictimVID(victim), dwAttackerVID(attacker), lDamage(damage), byDamageFlag(flag) {}
    };

    struct CombatFlyDomainEvent : public UserInterface::Core::IEvent
    {
        uint8_t byType;
        uint32_t dwStartVID;
        uint32_t dwEndVID;

        CombatFlyDomainEvent(uint8_t type, uint32_t startVID, uint32_t endVID)
            : byType(type), dwStartVID(startVID), dwEndVID(endVID) {}
    };

    struct CombatFlyTargetingDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwShooterVID;
        uint32_t dwTargetVID;
        int32_t lX;
        int32_t lY;

        CombatFlyTargetingDomainEvent(uint32_t shooter, uint32_t target, int32_t x, int32_t y)
            : dwShooterVID(shooter), dwTargetVID(target), lX(x), lY(y) {}
    };

    struct CombatDuelStartDomainEvent : public UserInterface::Core::IEvent
    {
        CombatDuelStartDomainEvent() = default;
    };

    /**
     * @brief Router domenowy odpowiedzialny za obsluge zdeserializowanych pakietow walki.
     * 
     * KONTRAKT ZERO-DESYNC:
     * Router NIGDY nie dotyka surowego strumienia TCP i NIGDY nie wywoluje Recv().
     * Przyjmuje wylacznie gotowe, zwalidowane rekordy POD przez const referencje.
     * Dziala wylacznie w glownym watku (single-threaded).
     */
    class NetCombatRouter : public UserInterface::Contracts::IPacketRouter
    {
    public:
        NetCombatRouter() noexcept = default;
        explicit NetCombatRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept;
        ~NetCombatRouter() override = default;

        // Implementacja kontraktu IPacketRouter
        [[nodiscard]] std::string_view GetRouterName() const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint8_t bHeader) const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint16_t wHeader) const noexcept;

        // Rejestracja odbiornika zdarzen domenowych
        void SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept;
        [[nodiscard]] UserInterface::Contracts::IGameEventSink* GetEventSink() const noexcept;

        // Metody dyspozycji zdeserializowanych pakietow walki
        void HandleAttack(const TPacketGCAttack& packet);
        void HandleDamageInfo(const TPacketGCDamageInfo& packet);
        void HandleFly(const TPacketGCCreateFly& packet);
        void HandleFlyTargeting(const TPacketGCFlyTargeting& packet);
        void HandleDuelStart(const TPacketGCDuelStart& packet);

    private:
        UserInterface::Contracts::IGameEventSink* m_pEventSink{nullptr};
    };
}

// Globalny alias w przestrzeni Network::Routers dla wygody
namespace Network::Routers
{
    using NetCombatRouter = UserInterface::Network::Routers::NetCombatRouter;
}
