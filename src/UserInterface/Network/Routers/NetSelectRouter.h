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
    // Zdarzenia domenowe publikowane na EventBus dla fazy wyboru i tworzenia postaci
    struct NetPlayerCreateSuccessDomainEvent : public UserInterface::Core::IEvent
    {
        uint8_t bSlot;
        std::string szName;

        NetPlayerCreateSuccessDomainEvent(uint8_t slot, std::string_view name)
            : bSlot(slot), szName(name) {}
    };

    struct NetPlayerCreateFailureDomainEvent : public UserInterface::Core::IEvent
    {
        uint8_t bType;

        explicit NetPlayerCreateFailureDomainEvent(uint8_t type)
            : bType(type) {}
    };

    struct NetPlayerDestroySuccessDomainEvent : public UserInterface::Core::IEvent
    {
        uint8_t bSlot;

        explicit NetPlayerDestroySuccessDomainEvent(uint8_t slot)
            : bSlot(slot) {}
    };

    struct NetChangeNameDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwPID;
        std::string szName;

        NetChangeNameDomainEvent(uint32_t pid, std::string_view name)
            : dwPID(pid), szName(name) {}
    };

    /**
     * @brief Router domenowy fazy wyboru i tworzenia postaci (Phase Select / Character).
     *
     * KONTRAKT ZERO-DESYNC:
     * Router przyjmuje wylacznie zdeserializowane rekordy POD przez const referencje.
     * Dziala wylacznie w glownym watku (single-threaded).
     */
    class NetSelectRouter : public UserInterface::Contracts::IPacketRouter
    {
    public:
        NetSelectRouter() noexcept = default;
        explicit NetSelectRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept;
        ~NetSelectRouter() override = default;

        // Implementacja kontraktu IPacketRouter
        [[nodiscard]] std::string_view GetRouterName() const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint8_t bHeader) const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint16_t wHeader) const noexcept;

        // Rejestracja odbiornika zdarzen domenowych
        void SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept;
        [[nodiscard]] UserInterface::Contracts::IGameEventSink* GetEventSink() const noexcept;

        // Metody dyspozycji zdeserializowanych pakietow wyboru/tworzenia postaci
        void HandlePlayerCreateSuccess(const TPacketGCPlayerCreateSuccess& packet);
        void HandlePlayerCreateFailure(const TPacketGCCreateFailure& packet);
        void HandlePlayerDestroySuccess(const TPacketGCDestroyCharacterSuccess& packet);
        void HandleChangeName(const TPacketGCChangeName& packet);
        void HandleEmpire(const TPacketGCEmpire& packet);

    private:
        UserInterface::Contracts::IGameEventSink* m_pEventSink{nullptr};
    };
}

namespace Network::Routers
{
    using NetSelectRouter = UserInterface::Network::Routers::NetSelectRouter;
}
