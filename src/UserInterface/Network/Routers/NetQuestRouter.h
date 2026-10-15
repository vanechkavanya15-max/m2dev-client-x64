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
    struct QuestInfoDomainEvent : public UserInterface::Core::IEvent
    {
        uint16_t wIndex;
        uint8_t byFlag;

        QuestInfoDomainEvent(uint16_t index, uint8_t flag)
            : wIndex(index), byFlag(flag) {}
    };

    struct QuestConfirmDomainEvent : public UserInterface::Core::IEvent
    {
        std::string strMsg;
        int32_t lTimeout;
        uint32_t dwRequestPID;

        QuestConfirmDomainEvent(std::string_view msg, int32_t timeout, uint32_t pid)
            : strMsg(msg), lTimeout(timeout), dwRequestPID(pid) {}
    };

    /**
     * @brief Router domenowy odpowiedzialny za obsluge zdeserializowanych pakietow zadan (Quest).
     * 
     * KONTRAKT ZERO-DESYNC:
     * Router NIGDY nie dotyka surowego bufora TCP i NIGDY nie wywoluje Recv().
     * Przyjmuje wylacznie zdeserializowane rekordy POD przez const referencje.
     * Dziala wylacznie w glownym watku (single-threaded).
     */
    class NetQuestRouter : public UserInterface::Contracts::IPacketRouter
    {
    public:
        NetQuestRouter() noexcept = default;
        explicit NetQuestRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept;
        ~NetQuestRouter() override = default;

        // Implementacja kontraktu IPacketRouter
        [[nodiscard]] std::string_view GetRouterName() const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint8_t bHeader) const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint16_t wHeader) const noexcept;

        // Rejestracja odbiornika zdarzen domenowych
        void SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept;
        [[nodiscard]] UserInterface::Contracts::IGameEventSink* GetEventSink() const noexcept;

        // Metody dyspozycji zdeserializowanych pakietow zadan
        void HandleQuestInfo(const TPacketGCQuestInfo& packet);
        void HandleQuestConfirm(const TPacketGCQuestConfirm& packet);

    private:
        UserInterface::Contracts::IGameEventSink* m_pEventSink{nullptr};
    };
}

// Globalny alias w przestrzeni Network::Routers dla wygody
namespace Network::Routers
{
    using NetQuestRouter = UserInterface::Network::Routers::NetQuestRouter;
}
