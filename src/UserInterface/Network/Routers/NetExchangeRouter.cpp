#if defined(_MSC_VER) && !defined(TEST_MODE_DISABLE_STDAFX)
#include "../../StdAfx.h"
#endif

#include "NetExchangeRouter.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Network::Routers
{
    NetExchangeRouter::NetExchangeRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept
        : m_pEventSink(pEventSink)
    {
    }

    std::string_view NetExchangeRouter::GetRouterName() const noexcept
    {
        return "NetExchangeRouter";
    }

    bool NetExchangeRouter::CanHandleHeader(uint8_t bHeader) const noexcept
    {
        switch (bHeader)
        {
        case 0x19: // GC::EXCHANGE (legacy 25)
        case 0x27: // GC::EXCHANGE (alternatywny)
            return true;
        default:
            return false;
        }
    }

    bool NetExchangeRouter::CanHandleHeader(uint16_t wHeader) const noexcept
    {
        switch (wHeader)
        {
        case 0x051C: // GC::EXCHANGE
            return true;
        default:
            if (wHeader <= 0xFF)
            {
                return CanHandleHeader(static_cast<uint8_t>(wHeader));
            }
            return false;
        }
    }

    void NetExchangeRouter::SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept
    {
        m_pEventSink = pSink;
    }

    UserInterface::Contracts::IGameEventSink* NetExchangeRouter::GetEventSink() const noexcept
    {
        return m_pEventSink;
    }

    void NetExchangeRouter::HandleExchange(const TPacketGCExchange& packet)
    {
        const bool bIsMe = (packet.is_me != 0);

        EterBase::ModernLogger::Debug("NetExchangeRouter: Obsluga Exchange [SubHeader: {}, IsMe: {}, Arg1: {}, Arg3: {}]",
            packet.subheader, bIsMe, packet.arg1, packet.arg3);

        if (m_pEventSink)
        {
            UserInterface::Contracts::ExchangeEvent event{};
            event.bySubHeader = packet.subheader;
            event.bIsMe = bIsMe;
            event.dwArg1 = packet.arg1;
            event.dwArg2 = static_cast<uint32_t>(packet.arg2.cell);
            event.dwArg3 = packet.arg3;
            m_pEventSink->OnExchange(event);
        }

        // Publikacja ogolnego zdarzenia wymiany
        UserInterface::Core::EventBus::GetInstance().Publish(
            ExchangeDomainEvent(packet.subheader, bIsMe, packet.arg1, packet.arg2, packet.arg3)
        );

        // Publikacja wyspecjalizowanych zdarzen domenowych w zaleznosci od subheader
        switch (packet.subheader)
        {
        case ExchangeSub::GC::START:
            UserInterface::Core::EventBus::GetInstance().Publish(
                ExchangeStartDomainEvent(bIsMe, packet.arg1)
            );
            break;

        case ExchangeSub::GC::ITEM_ADD:
            UserInterface::Core::EventBus::GetInstance().Publish(
                ExchangeItemAddDomainEvent(bIsMe, static_cast<uint8_t>(packet.arg2.cell), packet.arg2, packet.arg1)
            );
            break;

        case ExchangeSub::GC::ITEM_DEL:
            UserInterface::Core::EventBus::GetInstance().Publish(
                ExchangeItemDelDomainEvent(bIsMe, static_cast<uint8_t>(packet.arg1))
            );
            break;

        case ExchangeSub::GC::ELK_ADD:
            UserInterface::Core::EventBus::GetInstance().Publish(
                ExchangeElkAddDomainEvent(bIsMe, packet.arg1)
            );
            break;

        case ExchangeSub::GC::ACCEPT:
            UserInterface::Core::EventBus::GetInstance().Publish(
                ExchangeAcceptDomainEvent(bIsMe, static_cast<uint8_t>(packet.arg1))
            );
            break;

        case ExchangeSub::GC::END:
            UserInterface::Core::EventBus::GetInstance().Publish(
                ExchangeEndDomainEvent()
            );
            break;

        default:
            break;
        }
    }
}
