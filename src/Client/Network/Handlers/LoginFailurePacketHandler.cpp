#include "LoginFailurePacketHandler.h"
#include <cstring>
#include <algorithm>

#ifndef TEST_MODE_DISABLE_STDAFX
#include "EterBase/StdAfx.h"
#endif

#ifdef TEST_MODE_DISABLE_STDAFX
enum { LOGIN_STATUS_MAX_LEN = 8 };
typedef struct packet_login_failure
{
    uint16_t    header;
    uint16_t    length;
    char    szStatus[LOGIN_STATUS_MAX_LEN + 1];
} TPacketGCLoginFailure;
#endif

namespace Client::Network
{
    LoginFailurePacketHandler::LoginFailurePacketHandler(ILoginFailureCallback* callback) noexcept
        : m_callback(callback)
    {
    }

    EterBase::PacketResult<void> LoginFailurePacketHandler::Handle(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(TPacketGCLoginFailure))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCLoginFailure*>(payload.data());

        const char* end = std::find(packet->szStatus, packet->szStatus + sizeof(packet->szStatus), '\0');
        
        LoginFailureStatus status;
        status.status = std::string(packet->szStatus, static_cast<std::size_t>(std::distance(packet->szStatus, end)));

        if (m_callback)
        {
            m_callback->OnLoginFailure(status);
        }

        return {};
    }

    uint16_t LoginFailurePacketHandler::GetExpectedSize() const noexcept
    {
        return sizeof(TPacketGCLoginFailure);
    }

    bool LoginFailurePacketHandler::IsDynamicSize() const noexcept
    {
        return false;
    }
}
