#if defined(_MSC_VER) && !defined(TEST_MODE_DISABLE_STDAFX)
#include "../../StdAfx.h"
#endif

#include "NetShopRouter.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Network::Routers
{
    NetShopRouter::NetShopRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept
        : m_pEventSink(pEventSink)
    {
    }

    std::string_view NetShopRouter::GetRouterName() const noexcept
    {
        return "NetShopRouter";
    }

    bool NetShopRouter::CanHandleHeader(uint8_t bHeader) const noexcept
    {
        switch (bHeader)
        {
        case 0x26: // GC::SHOP (legacy 38)
        case 0x32: // GC::SHOP_SIGN (legacy 50)
            return true;
        default:
            return false;
        }
    }

    bool NetShopRouter::CanHandleHeader(uint16_t wHeader) const noexcept
    {
        switch (wHeader)
        {
        case 0x0802: // GC::MYSHOP
        case 0x0810: // GC::SHOP
        case 0x0811: // GC::SHOP_SIGN
            return true;
        default:
            if (wHeader <= 0xFF)
            {
                return CanHandleHeader(static_cast<uint8_t>(wHeader));
            }
            return false;
        }
    }

    void NetShopRouter::SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept
    {
        m_pEventSink = pSink;
    }

    UserInterface::Contracts::IGameEventSink* NetShopRouter::GetEventSink() const noexcept
    {
        return m_pEventSink;
    }

    void NetShopRouter::HandleShop(const TPacketGCShop& packet)
    {
        EterBase::ModernLogger::Debug("NetShopRouter: Obsluga Shop [SubHeader: {}, Length: {}]",
            packet.subheader, packet.length);

        if (m_pEventSink)
        {
            UserInterface::Contracts::ShopEvent event{};
            event.bySubHeader = packet.subheader;
            m_pEventSink->OnShop(event);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(
            ShopDomainEvent(packet.subheader)
        );
    }

    void NetShopRouter::HandleShopSign(const TPacketGCShopSign& packet)
    {
        EterBase::ModernLogger::Debug("NetShopRouter: Obsluga ShopSign [VID: {}, Sign: {}]",
            packet.dwVID, packet.szSign);

        if (m_pEventSink)
        {
            UserInterface::Contracts::ShopSignEvent event{};
            event.dwVID = packet.dwVID;
            event.szSign = packet.szSign;
            m_pEventSink->OnShopSign(event);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(
            ShopSignDomainEvent(packet.dwVID, packet.szSign)
        );
    }

    void NetShopRouter::HandleShopStart(const TPacketGCShopStart& /*packet*/)
    {
        EterBase::ModernLogger::Debug("NetShopRouter: Obsluga ShopStart");

        UserInterface::Core::EventBus::GetInstance().Publish(
            ShopStartDomainEvent()
        );
    }

    void NetShopRouter::HandleShopUpdateItem(const TPacketGCShopUpdateItem& packet)
    {
        EterBase::ModernLogger::Debug("NetShopRouter: Obsluga ShopUpdateItem [Pos: {}, Vnum: {}]",
            packet.pos, packet.item.vnum);

        UserInterface::Core::EventBus::GetInstance().Publish(
            ShopUpdateItemDomainEvent(packet.pos, packet)
        );
    }

    void NetShopRouter::HandleShopUpdatePrice(const TPacketGCShopUpdatePrice& packet)
    {
        EterBase::ModernLogger::Debug("NetShopRouter: Obsluga ShopUpdatePrice [Elk: {}]",
            packet.iElkAmount);

        UserInterface::Core::EventBus::GetInstance().Publish(
            ShopUpdatePriceDomainEvent(packet.iElkAmount)
        );
    }
}
