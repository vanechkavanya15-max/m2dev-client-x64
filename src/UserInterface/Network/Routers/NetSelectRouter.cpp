#if defined(_MSC_VER) && !defined(TEST_MODE_DISABLE_STDAFX)
#include "../../StdAfx.h"
#endif

#include "NetSelectRouter.h"
#include "NetLoginRouter.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Network::Routers
{
    NetSelectRouter::NetSelectRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept
        : m_pEventSink(pEventSink)
    {
    }

    std::string_view NetSelectRouter::GetRouterName() const noexcept
    {
        return "NetSelectRouter";
    }

    bool NetSelectRouter::CanHandleHeader(uint8_t bHeader) const noexcept
    {
        switch (bHeader)
        {
        case 0x11: // GC_PLAYER_CREATE_SUCCESS (legacy)
        case 0x12: // GC_PLAYER_CREATE_FAILURE (legacy)
        case 0x13: // GC_PLAYER_DELETE_SUCCESS (legacy)
        case 0x17: // GC_CHANGE_NAME (legacy)
        case 0x5A: // GC_EMPIRE (legacy)
            return true;
        default:
            return false;
        }
    }

    bool NetSelectRouter::CanHandleHeader(uint16_t wHeader) const noexcept
    {
        switch (wHeader)
        {
        case Client::Network::Protocol::GC::PLAYER_CREATE_SUCCESS:
        case Client::Network::Protocol::GC::PLAYER_CREATE_FAILURE:
        case Client::Network::Protocol::GC::PLAYER_DELETE_SUCCESS:
        case Client::Network::Protocol::GC::PLAYER_DELETE_WRONG_SOCIAL_ID:
        case Client::Network::Protocol::GC::CHANGE_NAME:
        case Client::Network::Protocol::GC::EMPIRE:
            return true;
        default:
            if (wHeader <= 0xFF)
            {
                return CanHandleHeader(static_cast<uint8_t>(wHeader));
            }
            return false;
        }
    }

    void NetSelectRouter::SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept
    {
        m_pEventSink = pSink;
    }

    UserInterface::Contracts::IGameEventSink* NetSelectRouter::GetEventSink() const noexcept
    {
        return m_pEventSink;
    }

    void NetSelectRouter::HandlePlayerCreateSuccess(const TPacketGCPlayerCreateSuccess& packet)
    {
        if (m_pEventSink)
        {
            UserInterface::Contracts::CharacterCreateSuccessEvent evt{
                packet.bAccountCharacterSlot,
                packet.kSimplePlayerInfomation.szName
            };
            m_pEventSink->OnCharacterCreateSuccess(evt);
        }

        UserInterface::Core::EventBus::Instance().Publish(
            NetPlayerCreateSuccessDomainEvent(packet.bAccountCharacterSlot, packet.kSimplePlayerInfomation.szName)
        );
    }

    void NetSelectRouter::HandlePlayerCreateFailure(const TPacketGCCreateFailure& packet)
    {
        if (m_pEventSink)
        {
            UserInterface::Contracts::CharacterCreateFailureEvent evt{
                packet.bType
            };
            m_pEventSink->OnCharacterCreateFailure(evt);
        }

        UserInterface::Core::EventBus::Instance().Publish(
            NetPlayerCreateFailureDomainEvent(packet.bType)
        );
    }

    void NetSelectRouter::HandlePlayerDestroySuccess(const TPacketGCDestroyCharacterSuccess& packet)
    {
        if (m_pEventSink)
        {
            UserInterface::Contracts::CharacterDeleteSuccessEvent evt{
                static_cast<uint8_t>(packet.account_index)
            };
            m_pEventSink->OnCharacterDeleteSuccess(evt);
        }

        UserInterface::Core::EventBus::Instance().Publish(
            NetPlayerDestroySuccessDomainEvent(static_cast<uint8_t>(packet.account_index))
        );
    }

    void NetSelectRouter::HandleChangeName(const TPacketGCChangeName& packet)
    {
        UserInterface::Core::EventBus::Instance().Publish(
            NetChangeNameDomainEvent(packet.pid, packet.name)
        );
    }

    void NetSelectRouter::HandleEmpire(const TPacketGCEmpire& packet)
    {
        if (m_pEventSink)
        {
            UserInterface::Contracts::EmpireSelectedEvent evt{
                packet.bEmpire
            };
            m_pEventSink->OnEmpireSelected(evt);
        }

        UserInterface::Core::EventBus::Instance().Publish(
            NetEmpireDomainEvent(packet.bEmpire)
        );
    }
}
