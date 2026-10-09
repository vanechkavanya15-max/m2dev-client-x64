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
    struct ShopDomainEvent : public UserInterface::Core::IEvent
    {
        uint8_t bySubHeader;

        explicit ShopDomainEvent(uint8_t subheader) : bySubHeader(subheader) {}
    };

    struct ShopSignDomainEvent : public UserInterface::Core::IEvent
    {
        uint32_t dwVID;
        std::string strSign;

        ShopSignDomainEvent(uint32_t vid, std::string_view sign)
            : dwVID(vid), strSign(sign) {}
    };

    struct ShopStartDomainEvent : public UserInterface::Core::IEvent
    {
        ShopStartDomainEvent() = default;
    };

    struct ShopUpdateItemDomainEvent : public UserInterface::Core::IEvent
    {
        uint8_t byPos;
        TPacketGCShopUpdateItem item;

        ShopUpdateItemDomainEvent(uint8_t pos, const TPacketGCShopUpdateItem& updateItem)
            : byPos(pos), item(updateItem) {}
    };

    struct ShopUpdatePriceDomainEvent : public UserInterface::Core::IEvent
    {
        int32_t lElkAmount;

        explicit ShopUpdatePriceDomainEvent(int32_t elk) : lElkAmount(elk) {}
    };

    /**
     * @brief Router domenowy odpowiedzialny za obsluge zdeserializowanych pakietow sklepu (Shop).
     * 
     * KONTRAKT ZERO-DESYNC:
     * Router NIGDY nie dotyka surowego bufora TCP i NIGDY nie wywoluje Recv().
     * Przyjmuje wylacznie zdeserializowane rekordy POD przez const referencje.
     * Dziala wylacznie w glownym watku (single-threaded).
     */
    class NetShopRouter : public UserInterface::Contracts::IPacketRouter
    {
    public:
        NetShopRouter() noexcept = default;
        explicit NetShopRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept;
        ~NetShopRouter() override = default;

        // Implementacja kontraktu IPacketRouter
        [[nodiscard]] std::string_view GetRouterName() const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint8_t bHeader) const noexcept override;
        [[nodiscard]] bool CanHandleHeader(uint16_t wHeader) const noexcept;

        // Rejestracja odbiornika zdarzen domenowych
        void SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept;
        [[nodiscard]] UserInterface::Contracts::IGameEventSink* GetEventSink() const noexcept;

        // Metody dyspozycji zdeserializowanych pakietow sklepu
        void HandleShop(const TPacketGCShop& packet);
        void HandleShopSign(const TPacketGCShopSign& packet);
        void HandleShopStart(const TPacketGCShopStart& packet);
        void HandleShopUpdateItem(const TPacketGCShopUpdateItem& packet);
        void HandleShopUpdatePrice(const TPacketGCShopUpdatePrice& packet);

    private:
        UserInterface::Contracts::IGameEventSink* m_pEventSink{nullptr};
    };
}

// Globalny alias w przestrzeni Network::Routers dla wygody
namespace Network::Routers
{
    using NetShopRouter = UserInterface::Network::Routers::NetShopRouter;
}
