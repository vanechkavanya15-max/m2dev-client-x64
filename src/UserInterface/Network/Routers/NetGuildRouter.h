#pragma once

#include "../../Contracts/IPacketRouter.h"
#include "../../Contracts/IGameEvents.h"
#include "Client/Network/Protocol/Protocol.h"
#include "../../Core/EventBus.h"
#include <string_view>
#include <cstdint>

namespace UserInterface::Network::Routers
{
    // Struktury zdarzen domenowych dla szyny EventBus
    struct GuildDomainEvent : public UserInterface::Core::IEvent
    {
        uint8_t bySubHeader;

        explicit GuildDomainEvent(uint8_t subheader) : bySubHeader(subheader) {}
    };

    struct GuildWarDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwGuildSelf;
        uint32_t dwGuildOpp;
        uint8_t byType;
        uint8_t byWarState;

        GuildWarDomainEvent(uint32_t self, uint32_t opp, uint8_t type, uint8_t state)
            : dwGuildSelf(self), dwGuildOpp(opp), byType(type), byWarState(state) {}
    };

    struct GuildWarPointDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwGainGuildID;
        uint32_t dwOpponentGuildID;
        int32_t lPoint;

        GuildWarPointDomainEvent(uint32_t gain, uint32_t opp, int32_t point)
            : dwGainGuildID(gain), dwOpponentGuildID(opp), lPoint(point) {}
    };

    /**
     * @brief Router domenowy odpowiedzialny za obsluge zdeserializowanych pakietow gildii (Guild).
     * 
     * KONTRAKT ZERO-DESYNC:
     * Router NIGDY nie dotyka surowego bufora TCP i NIGDY nie wywoluje Recv().
     * Przyjmuje wylacznie zdeserializowane rekordy POD przez const referencje.
     * Dziala wylacznie w glownym watku (single-threaded).
     */
    class NetGuildRouter : public UserInterface::Contracts::IPacketRouter
    {
    public:
        NetGuildRouter() noexcept = default;
        explicit NetGuildRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept;
        ~NetGuildRouter() override = default;

        // Implementacja kontraktu IPacketRouter
        [[nodiscard]] std::string_view GetRouterName() const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint8_t bHeader) const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint16_t wHeader) const noexcept;

        // Rejestracja odbiornika zdarzen domenowych
        void SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept;
        [[nodiscard]] UserInterface::Contracts::IGameEventSink* GetEventSink() const noexcept;

        // Metody dyspozycji zdeserializowanych pakietow gildii
        void HandleGuild(const TPacketGCGuild& packet);
        void HandleGuildWar(const TPacketGCGuildWar& packet);
        void HandleGuildWarPoint(const TPacketGuildWarPoint& packet);

    private:
        UserInterface::Contracts::IGameEventSink* m_pEventSink{nullptr};
    };
}

// Globalny alias w przestrzeni Network::Routers dla wygody
namespace Network::Routers
{
    using NetGuildRouter = UserInterface::Network::Routers::NetGuildRouter;
}
