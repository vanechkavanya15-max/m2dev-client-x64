#if defined(_MSC_VER) && !defined(TEST_MODE_DISABLE_STDAFX)
#include "../../StdAfx.h"
#endif

#include "NetLoginRouter.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Network::Routers
{
    NetLoginRouter::NetLoginRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept
        : m_pEventSink(pEventSink)
    {
    }

    std::string_view NetLoginRouter::GetRouterName() const noexcept
    {
        return "NetLoginRouter";
    }

    bool NetLoginRouter::CanHandleHeader(uint8_t bHeader) const noexcept
    {
        switch (bHeader)
        {
        case 0x07: // GC_LOGIN_SUCCESS3 (legacy)
        case 0x08: // GC_LOGIN_FAILURE (legacy)
        case 0x09: // GC_LOGIN_KEY (legacy)
        case 0x5A: // GC_EMPIRE (legacy)
            return true;
        default:
            return false;
        }
    }

    bool NetLoginRouter::CanHandleHeader(uint16_t wHeader) const noexcept
    {
        switch (wHeader)
        {
        case Client::Network::Protocol::GC::LOGIN_SUCCESS3:
        case Client::Network::Protocol::GC::LOGIN_SUCCESS4:
        case Client::Network::Protocol::GC::LOGIN_FAILURE:
        case Client::Network::Protocol::GC::LOGIN_KEY:
        case Client::Network::Protocol::GC::AUTH_SUCCESS:
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

    void NetLoginRouter::SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept
    {
        m_pEventSink = pSink;
    }

    UserInterface::Contracts::IGameEventSink* NetLoginRouter::GetEventSink() const noexcept
    {
        return m_pEventSink;
    }

    void NetLoginRouter::HandleLoginSuccess3(const TPacketGCLoginSuccess3& packet)
    {
        if (m_pEventSink)
        {
            UserInterface::Contracts::LoginSuccessEvent evt{
                packet.handle,
                packet.random_key,
                PLAYER_PER_ACCOUNT3
            };
            m_pEventSink->OnLoginSuccess(evt);
        }

        UserInterface::Core::EventBus::Instance().Publish(
            NetLoginSuccessDomainEvent(packet.handle, packet.random_key, PLAYER_PER_ACCOUNT3)
        );
    }

    void NetLoginRouter::HandleLoginSuccess4(const TPacketGCLoginSuccess4& packet)
    {
        if (m_pEventSink)
        {
            UserInterface::Contracts::LoginSuccessEvent evt{
                packet.handle,
                packet.random_key,
                PLAYER_PER_ACCOUNT4
            };
            m_pEventSink->OnLoginSuccess(evt);
        }

        UserInterface::Core::EventBus::Instance().Publish(
            NetLoginSuccessDomainEvent(packet.handle, packet.random_key, PLAYER_PER_ACCOUNT4)
        );
    }

    void NetLoginRouter::HandleLoginFailure(const TPacketGCLoginFailure& packet)
    {
        if (m_pEventSink)
        {
            UserInterface::Contracts::LoginFailureEvent evt{
                packet.szStatus
            };
            m_pEventSink->OnLoginFailure(evt);
        }

        UserInterface::Core::EventBus::Instance().Publish(
            NetLoginFailureDomainEvent(packet.szStatus)
        );
    }

    void NetLoginRouter::HandleLoginKey(const TPacketGCLoginKey& packet)
    {
        // Obsluga synchronizacji klucza sesji logowania
        EterBase::ModernLogger::Debug("NetLoginRouter: Otrzymano klucz logowania: 0x{:08X}", packet.dwLoginKey);
    }

    void NetLoginRouter::HandleEmpire(const TPacketGCEmpire& packet)
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
