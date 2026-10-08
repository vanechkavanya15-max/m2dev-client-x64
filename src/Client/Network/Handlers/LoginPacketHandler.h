#pragma once


#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include "../../../EterBase/Result.h"
#include "../../Network/ModernPacketDispatcher.h"
#ifndef TEST_MODE_DISABLE_STDAFX
#include "../Protocol/Protocol.h"
#endif

namespace Client::Network
{
    struct SessionKey
    {
        uint32_t loginKey;
        uint32_t handle;
        uint32_t randomKey;
    };

    struct AccountStatus
    {
        std::string status;
    };

    struct PlayerInfo
    {
        uint32_t id;
        std::string name;
        uint8_t job;
        uint8_t level;
        uint32_t playMinutes;
        uint8_t st, ht, dx, iq;
        uint16_t mainPart;
        uint8_t changeName;
        uint16_t hairPart;
        int32_t x, y;
        uint32_t addr;
        uint16_t port;
        uint8_t skillGroup;
        uint32_t guildId;
        std::string guildName;
    };

    struct LoginResult
    {
        std::vector<PlayerInfo> players;
        SessionKey session;
    };

    class ILoginCallback
    {
    public:
        virtual ~ILoginCallback() = default;

        virtual void OnLoginSuccess(const LoginResult& result) = 0;
        virtual void OnLoginFailure(const AccountStatus& status) = 0;
        virtual void OnAuthSuccess(const SessionKey& key) = 0;
    };

    class LoginPacketHandler : public IPacketHandler
    {
    public:
        explicit LoginPacketHandler(ILoginCallback* callback);
        ~LoginPacketHandler() override = default;

        [[nodiscard]] EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) override;
        [[nodiscard]] uint16_t GetExpectedSize() const override;
        [[nodiscard]] bool IsDynamicSize() const override;

    private:
        [[nodiscard]] EterBase::PacketResult<void> HandleLoginSuccess4(std::span<const uint8_t> payload);
        [[nodiscard]] EterBase::PacketResult<void> HandleLoginFailure(std::span<const uint8_t> payload);
        [[nodiscard]] EterBase::PacketResult<void> HandleAuthSuccess(std::span<const uint8_t> payload);

        ILoginCallback* m_callback;
    };
}
