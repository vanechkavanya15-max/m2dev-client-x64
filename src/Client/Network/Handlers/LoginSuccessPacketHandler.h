#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include "../../../EterBase/Result.h"

namespace Client::Network::Handlers {

    // Stale zgodne z bazowym kodem
    constexpr size_t CHARACTER_NAME_MAX_LEN_CONST = 24;
    constexpr size_t GUILD_NAME_MAX_LEN_CONST = 12;
    constexpr size_t PLAYER_PER_ACCOUNT4_CONST = 4;

    struct PlayerSlotData {
        uint32_t id;
        std::string name;
        uint8_t job;
        uint8_t level;
        uint32_t playMinutes;
        uint8_t st;
        uint8_t ht;
        uint8_t dx;
        uint8_t iq;
        uint16_t mainPart;
        uint8_t changeName;
        uint16_t hairPart;
        int32_t x;
        int32_t y;
        uint32_t addr;
        uint16_t port;
        uint8_t skillGroup;
        uint32_t guildId;
        std::string guildName;
    };

    struct SessionKeyData {
        uint32_t handle;
        uint32_t randomKey;
    };

    struct LoginSuccessData {
        std::vector<PlayerSlotData> players;
        SessionKeyData sessionKey;
    };

    class LoginSuccessPacketHandler {
    public:
        using CallbackType = void(*)(const LoginSuccessData&);

        explicit LoginSuccessPacketHandler(CallbackType callback = nullptr) noexcept;
        ~LoginSuccessPacketHandler() = default;

        // Blokada kopiowania/przenoszenia dla handlerow bezpiecznych sesyjnie
        LoginSuccessPacketHandler(const LoginSuccessPacketHandler&) = delete;
        LoginSuccessPacketHandler& operator=(const LoginSuccessPacketHandler&) = delete;
        LoginSuccessPacketHandler(LoginSuccessPacketHandler&&) = delete;
        LoginSuccessPacketHandler& operator=(LoginSuccessPacketHandler&&) = delete;

        [[nodiscard]] EterBase::PacketResult<void> ProcessPacket(std::span<const uint8_t> payload);

    private:
        CallbackType m_callback{nullptr};
    };

} // namespace Client::Network::Handlers
