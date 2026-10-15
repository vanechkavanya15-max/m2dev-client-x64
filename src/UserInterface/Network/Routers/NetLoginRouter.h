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
    // Zdarzenia publikowane na EventBus dla fazy logowania
    struct NetLoginSuccessDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwHandle;
        uint32_t dwRandomKey;
        uint32_t dwPlayerCount;

        NetLoginSuccessDomainEvent(uint32_t handle, uint32_t randomKey, uint32_t playerCount)
            : dwHandle(handle), dwRandomKey(randomKey), dwPlayerCount(playerCount) {}
    };

    struct NetLoginFailureDomainEvent : public UserInterface::Core::IEvent
    {
        std::string szStatus;

        explicit NetLoginFailureDomainEvent(std::string_view status)
            : szStatus(status) {}
    };

    struct NetEmpireDomainEvent : public UserInterface::Core::IEvent
    {
        uint8_t byEmpire;

        explicit NetEmpireDomainEvent(uint8_t empire)
            : byEmpire(empire) {}
    };

    /**
     * @brief Router domenowy fazy logowania i autoryzacji (Phase Login / Auth).
     *
     * KONTRAKT ZERO-DESYNC:
     * Router przyjmuje wylacznie zdeserializowane rekordy POD przez const referencje.
     * Dziala wylacznie w glownym watku (single-threaded).
     */
    class NetLoginRouter : public UserInterface::Contracts::IPacketRouter
    {
    public:
        NetLoginRouter() noexcept = default;
        explicit NetLoginRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept;
        ~NetLoginRouter() override = default;

        // Implementacja kontraktu IPacketRouter
        [[nodiscard]] std::string_view GetRouterName() const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint8_t bHeader) const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint16_t wHeader) const noexcept;

        // Rejestracja odbiornika zdarzen domenowych
        void SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept;
        [[nodiscard]] UserInterface::Contracts::IGameEventSink* GetEventSink() const noexcept;

        // Metody dyspozycji zdeserializowanych pakietow logowania
        void HandleLoginSuccess3(const TPacketGCLoginSuccess3& packet);
        void HandleLoginSuccess4(const TPacketGCLoginSuccess4& packet);
        void HandleLoginFailure(const TPacketGCLoginFailure& packet);
        void HandleLoginKey(const TPacketGCLoginKey& packet);
        void HandleEmpire(const TPacketGCEmpire& packet);

    private:
        UserInterface::Contracts::IGameEventSink* m_pEventSink{nullptr};
    };
}

namespace Network::Routers
{
    using NetLoginRouter = UserInterface::Network::Routers::NetLoginRouter;
}
