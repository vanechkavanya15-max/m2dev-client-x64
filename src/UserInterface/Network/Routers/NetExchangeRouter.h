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
    struct ExchangeDomainEvent : public UserInterface::Core::IEvent
    {
        uint8_t bySubHeader;
        bool bIsMe;
        uint32_t dwArg1;
        TItemPos Cell;
        uint32_t dwArg3;

        ExchangeDomainEvent(uint8_t subheader, bool isMe, uint32_t arg1, const TItemPos& pos, uint32_t arg3)
            : bySubHeader(subheader), bIsMe(isMe), dwArg1(arg1), Cell(pos), dwArg3(arg3) {}
    };

    struct ExchangeStartDomainEvent : public UserInterface::Core::IEvent
    {
        bool bIsMe;
        uint32_t dwTargetVID;

        ExchangeStartDomainEvent(bool isMe, uint32_t targetVid)
            : bIsMe(isMe), dwTargetVID(targetVid) {}
    };

    struct ExchangeItemAddDomainEvent : public UserInterface::Core::IEvent
    {
        bool bIsMe;
        uint8_t byDisplayPos;
        TItemPos Cell;
        uint32_t dwVnum;

        ExchangeItemAddDomainEvent(bool isMe, uint8_t displayPos, const TItemPos& pos, uint32_t vnum)
            : bIsMe(isMe), byDisplayPos(displayPos), Cell(pos), dwVnum(vnum) {}
    };

    struct ExchangeItemDelDomainEvent : public UserInterface::Core::IEvent
    {
        bool bIsMe;
        uint8_t byDisplayPos;

        ExchangeItemDelDomainEvent(bool isMe, uint8_t displayPos)
            : bIsMe(isMe), byDisplayPos(displayPos) {}
    };

    struct ExchangeElkAddDomainEvent : public UserInterface::Core::IEvent
    {
        bool bIsMe;
        uint32_t dwElk;

        ExchangeElkAddDomainEvent(bool isMe, uint32_t elk)
            : bIsMe(isMe), dwElk(elk) {}
    };

    struct ExchangeAcceptDomainEvent : public UserInterface::Core::IEvent
    {
        bool bIsMe;
        uint8_t byAccept;

        ExchangeAcceptDomainEvent(bool isMe, uint8_t accept)
            : bIsMe(isMe), byAccept(accept) {}
    };

    struct ExchangeEndDomainEvent : public UserInterface::Core::IEvent
    {
        ExchangeEndDomainEvent() = default;
    };

    /**
     * @brief Router domenowy odpowiedzialny za obsluge zdeserializowanych pakietow handlu p2p (Exchange).
     * 
     * KONTRAKT ZERO-DESYNC:
     * Router NIGDY nie dotyka surowego bufora TCP i NIGDY nie wywoluje Recv().
     * Przyjmuje wylacznie zdeserializowane rekordy POD przez const referencje.
     * Dziala wylacznie w glownym watku (single-threaded).
     */
    class NetExchangeRouter : public UserInterface::Contracts::IPacketRouter
    {
    public:
        NetExchangeRouter() noexcept = default;
        explicit NetExchangeRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept;
        ~NetExchangeRouter() override = default;

        // Implementacja kontraktu IPacketRouter
        [[nodiscard]] std::string_view GetRouterName() const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint8_t bHeader) const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint16_t wHeader) const noexcept;

        // Rejestracja odbiornika zdarzen domenowych
        void SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept;
        [[nodiscard]] UserInterface::Contracts::IGameEventSink* GetEventSink() const noexcept;

        // Metody dyspozycji zdeserializowanych pakietow handlu p2p
        void HandleExchange(const TPacketGCExchange& packet);

    private:
        UserInterface::Contracts::IGameEventSink* m_pEventSink{nullptr};
    };
}

// Globalny alias w przestrzeni Network::Routers dla wygody
namespace Network::Routers
{
    using NetExchangeRouter = UserInterface::Network::Routers::NetExchangeRouter;
}
