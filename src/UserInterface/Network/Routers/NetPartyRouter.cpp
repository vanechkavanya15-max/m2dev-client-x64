#if defined(_MSC_VER) && !defined(TEST_MODE_DISABLE_STDAFX)
#include "../../StdAfx.h"
#endif

#include "NetPartyRouter.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Network::Routers
{
    NetPartyRouter::NetPartyRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept
        : m_pEventSink(pEventSink)
    {
    }

    std::string_view NetPartyRouter::GetRouterName() const noexcept
    {
        return "NetPartyRouter";
    }

    bool NetPartyRouter::CanHandleHeader(uint8_t bHeader) const noexcept
    {
        switch (bHeader)
        {
        case 0x2E: // GC::PARTY_INVITE (legacy 46)
        case 0x2F: // GC::PARTY_ADD (legacy 47)
        case 0x30: // GC::PARTY_UPDATE (legacy 48)
        case 0x31: // GC::PARTY_REMOVE (legacy 49)
        case 0x34: // GC::PARTY_LINK (legacy 52)
        case 0x35: // GC::PARTY_UNLINK (legacy 53)
        case 0x3B: // GC::PARTY_PARAMETER (legacy 59)
            return true;
        default:
            return false;
        }
    }

    bool NetPartyRouter::CanHandleHeader(uint16_t wHeader) const noexcept
    {
        switch (wHeader)
        {
        case 0x0710: // GC::PARTY_INVITE
        case 0x0711: // GC::PARTY_ADD
        case 0x0712: // GC::PARTY_UPDATE
        case 0x0713: // GC::PARTY_REMOVE
        case 0x0714: // GC::PARTY_LINK
        case 0x0715: // GC::PARTY_UNLINK
        case 0x0716: // GC::PARTY_PARAMETER
            return true;
        default:
            if (wHeader <= 0xFF)
            {
                return CanHandleHeader(static_cast<uint8_t>(wHeader));
            }
            return false;
        }
    }

    void NetPartyRouter::SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept
    {
        m_pEventSink = pSink;
    }

    UserInterface::Contracts::IGameEventSink* NetPartyRouter::GetEventSink() const noexcept
    {
        return m_pEventSink;
    }

    void NetPartyRouter::HandlePartyInvite(const TPacketGCPartyInvite& packet)
    {
        EterBase::ModernLogger::Debug("NetPartyRouter: Obsluga PartyInvite [LeaderPID: {}]",
            packet.leader_pid);

        UserInterface::Core::EventBus::GetInstance().Publish(
            PartyInviteDomainEvent(packet.leader_pid)
        );
    }

    void NetPartyRouter::HandlePartyAdd(const TPacketGCPartyAdd& packet)
    {
        EterBase::ModernLogger::Debug("NetPartyRouter: Obsluga PartyAdd [PID: {}, Name: {}]",
            packet.pid, packet.name);

        if (m_pEventSink)
        {
            UserInterface::Contracts::PartyAddEvent event{};
            event.dwPID = packet.pid;
            event.szName = packet.name;
            m_pEventSink->OnPartyAdd(event);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(
            PartyAddDomainEvent(packet.pid, packet.name)
        );
    }

    void NetPartyRouter::HandlePartyUpdate(const TPacketGCPartyUpdate& packet)
    {
        EterBase::ModernLogger::Debug("NetPartyRouter: Obsluga PartyUpdate [PID: {}, State: {}, HP: {}%]",
            packet.pid, packet.state, packet.percent_hp);

        if (m_pEventSink)
        {
            UserInterface::Contracts::PartyUpdateEvent event{};
            event.dwPID = packet.pid;
            event.byState = packet.state;
            event.byPercentHP = packet.percent_hp;
            m_pEventSink->OnPartyUpdate(event);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(
            PartyUpdateDomainEvent(packet.pid, packet.state, packet.percent_hp)
        );
    }

    void NetPartyRouter::HandlePartyRemove(const TPacketGCPartyRemove& packet)
    {
        EterBase::ModernLogger::Debug("NetPartyRouter: Obsluga PartyRemove [PID: {}]",
            packet.pid);

        if (m_pEventSink)
        {
            UserInterface::Contracts::PartyRemoveEvent event{};
            event.dwPID = packet.pid;
            m_pEventSink->OnPartyRemove(event);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(
            PartyRemoveDomainEvent(packet.pid)
        );
    }

    void NetPartyRouter::HandlePartyParameter(const TPacketGCPartyParameter& /*packet*/)
    {
        EterBase::ModernLogger::Debug("NetPartyRouter: Obsluga PartyParameter");

        UserInterface::Core::EventBus::GetInstance().Publish(
            PartyParameterDomainEvent()
        );
    }
}
