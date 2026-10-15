#pragma once

#include "../../Contracts/IPacketRouter.h"
#include "../../Contracts/IGameEvents.h"
#include "Client/Network/Protocol/Protocol.h"
#include "../../Core/EventBus.h"
#include <string_view>
#include <string>
#include <cstdint>

namespace UserInterface::Network::Routers
{
    // Struktury zdarzen domenowych dla szyny EventBus
    struct PartyInviteDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwLeaderPID;

        explicit PartyInviteDomainEvent(uint32_t pid) : dwLeaderPID(pid) {}
    };

    struct PartyAddDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwPID;
        std::string strName;

        PartyAddDomainEvent(uint32_t pid, std::string_view name)
            : dwPID(pid), strName(name) {}
    };

    struct PartyUpdateDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwPID;
        uint8_t byState;
        uint8_t byPercentHP;

        PartyUpdateDomainEvent(uint32_t pid, uint8_t state, uint8_t hp)
            : dwPID(pid), byState(state), byPercentHP(hp) {}
    };

    struct PartyRemoveDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwPID;

        explicit PartyRemoveDomainEvent(uint32_t pid) : dwPID(pid) {}
    };

    struct PartyParameterDomainEvent : public UserInterface::Core::IEvent
    {
        PartyParameterDomainEvent() = default;
    };

    /**
     * @brief Router domenowy odpowiedzialny za obsluge zdeserializowanych pakietow druzyny (Party).
     * 
     * KONTRAKT ZERO-DESYNC:
     * Router NIGDY nie dotyka surowego bufora TCP i NIGDY nie wywoluje Recv().
     * Przyjmuje wylacznie zdeserializowane rekordy POD przez const referencje.
     * Dziala wylacznie w glownym watku (single-threaded).
     */
    class NetPartyRouter : public UserInterface::Contracts::IPacketRouter
    {
    public:
        NetPartyRouter() noexcept = default;
        explicit NetPartyRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept;
        ~NetPartyRouter() override = default;

        // Implementacja kontraktu IPacketRouter
        [[nodiscard]] std::string_view GetRouterName() const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint8_t bHeader) const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint16_t wHeader) const noexcept;

        // Rejestracja odbiornika zdarzen domenowych
        void SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept;
        [[nodiscard]] UserInterface::Contracts::IGameEventSink* GetEventSink() const noexcept;

        // Metody dyspozycji zdeserializowanych pakietow druzyny
        void HandlePartyInvite(const TPacketGCPartyInvite& packet);
        void HandlePartyAdd(const TPacketGCPartyAdd& packet);
        void HandlePartyUpdate(const TPacketGCPartyUpdate& packet);
        void HandlePartyRemove(const TPacketGCPartyRemove& packet);
        void HandlePartyParameter(const TPacketGCPartyParameter& packet);

    private:
        UserInterface::Contracts::IGameEventSink* m_pEventSink{nullptr};
    };
}

// Globalny alias w przestrzeni Network::Routers dla wygody
namespace Network::Routers
{
    using NetPartyRouter = UserInterface::Network::Routers::NetPartyRouter;
}
