#if defined(_MSC_VER) && !defined(TEST_MODE_DISABLE_STDAFX)
#include "../../StdAfx.h"
#endif

#include "NetGuildRouter.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Network::Routers
{
    NetGuildRouter::NetGuildRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept
        : m_pEventSink(pEventSink)
    {
    }

    std::string_view NetGuildRouter::GetRouterName() const noexcept
    {
        return "NetGuildRouter";
    }

    bool NetGuildRouter::CanHandleHeader(uint8_t bHeader) const noexcept
    {
        switch (bHeader)
        {
        case 0x33: // GC::GUILD (legacy 51)
        case 0x38: // GC::REQUEST_MAKE_GUILD (legacy 56)
            return true;
        default:
            return false;
        }
    }

    bool NetGuildRouter::CanHandleHeader(uint16_t wHeader) const noexcept
    {
        switch (wHeader)
        {
        case 0x0730: // GC::GUILD
        case 0x0731: // GC::REQUEST_MAKE_GUILD
        case 0x0732: // GC::SYMBOL_DATA
            return true;
        default:
            if (wHeader <= 0xFF)
            {
                return CanHandleHeader(static_cast<uint8_t>(wHeader));
            }
            return false;
        }
    }

    void NetGuildRouter::SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept
    {
        m_pEventSink = pSink;
    }

    UserInterface::Contracts::IGameEventSink* NetGuildRouter::GetEventSink() const noexcept
    {
        return m_pEventSink;
    }

    void NetGuildRouter::HandleGuild(const TPacketGCGuild& packet)
    {
        EterBase::ModernLogger::Debug("NetGuildRouter: Obsluga Guild [SubHeader: {}, Length: {}]",
            packet.subheader, packet.length);

        if (m_pEventSink)
        {
            UserInterface::Contracts::GuildEvent event{};
            event.bySubHeader = packet.subheader;
            m_pEventSink->OnGuild(event);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(
            GuildDomainEvent(packet.subheader)
        );
    }

    void NetGuildRouter::HandleGuildWar(const TPacketGCGuildWar& packet)
    {
        EterBase::ModernLogger::Debug("NetGuildRouter: Obsluga GuildWar [Self: {}, Opp: {}, State: {}]",
            packet.dwGuildSelf, packet.dwGuildOpp, packet.bWarState);

        UserInterface::Core::EventBus::GetInstance().Publish(
            GuildWarDomainEvent(packet.dwGuildSelf, packet.dwGuildOpp, packet.bType, packet.bWarState)
        );
    }

    void NetGuildRouter::HandleGuildWarPoint(const TPacketGuildWarPoint& packet)
    {
        EterBase::ModernLogger::Debug("NetGuildRouter: Obsluga GuildWarPoint [Gain: {}, Opp: {}, Point: {}]",
            packet.dwGainGuildID, packet.dwOpponentGuildID, packet.lPoint);

        UserInterface::Core::EventBus::GetInstance().Publish(
            GuildWarPointDomainEvent(packet.dwGainGuildID, packet.dwOpponentGuildID, packet.lPoint)
        );
    }
}
