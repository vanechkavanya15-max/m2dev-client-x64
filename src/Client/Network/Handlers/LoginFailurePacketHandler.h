#pragma once

#include <cstdint>
#include <span>
#include <string>
#include "../../../EterBase/Result.h"
#include "../../Network/ModernPacketDispatcher.h"

namespace Client::Network
{
    struct LoginFailureStatus
    {
        std::string status;
    };

    class ILoginFailureCallback
    {
    public:
        virtual ~ILoginFailureCallback() = default;
        virtual void OnLoginFailure(const LoginFailureStatus& status) = 0;
    };

    class LoginFailurePacketHandler : public IPacketHandler
    {
    public:
        explicit LoginFailurePacketHandler(ILoginFailureCallback* callback) noexcept;
        ~LoginFailurePacketHandler() override = default;

        [[nodiscard]] EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) override;
        [[nodiscard]] uint16_t GetExpectedSize() const noexcept override;
        [[nodiscard]] bool IsDynamicSize() const noexcept override;

    private:
        ILoginFailureCallback* m_callback;
    };
}
