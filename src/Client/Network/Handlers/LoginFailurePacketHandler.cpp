#include "LoginFailurePacketHandler.h"
#include <cstring>
#include <algorithm>

#include "../Protocol/Protocol.h"

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

        size_t statusLen = strnlen(packet->szStatus, sizeof(packet->szStatus));
        LoginFailureStatus status;
        status.status = std::string(packet->szStatus, statusLen);

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
