#include "LoginSuccessPacketHandler.h"
#include <cstring>
#include <algorithm>

namespace Client::Network::Handlers {

    // Struktury mapujace dokladny uklad binarny zgodny z `TPacketGCLoginSuccess4` w `LoginPacketHandler.cpp`
#pragma pack(push, 1)
    struct RawSimplePlayerInformation {
        uint32_t dwID;
        char szName[CHARACTER_NAME_MAX_LEN_CONST + 1];
        uint8_t byJob;
        uint8_t byLevel;
        uint32_t dwPlayMinutes;
        uint8_t byST;
        uint8_t byHT;
        uint8_t byDX;
        uint8_t byIQ;
        uint16_t wMainPart;
        uint8_t bChangeName;
        uint16_t wHairPart;
        uint8_t bDummy[4];
        int32_t x;
        int32_t y;
        uint32_t lAddr;
        uint16_t wPort;
        uint8_t bySkillGroup;
    };

    struct RawPacketLoginSuccess4 {
        uint16_t header;
        uint16_t length;
        RawSimplePlayerInformation akSimplePlayerInformation[PLAYER_PER_ACCOUNT4_CONST];
        uint32_t guild_id[PLAYER_PER_ACCOUNT4_CONST];
        char guild_name[PLAYER_PER_ACCOUNT4_CONST][GUILD_NAME_MAX_LEN_CONST + 1];
        uint32_t handle;
        uint32_t random_key;
    };
#pragma pack(pop)

    LoginSuccessPacketHandler::LoginSuccessPacketHandler(CallbackType callback) noexcept
        : m_callback(callback)
    {
    }

    EterBase::PacketResult<void> LoginSuccessPacketHandler::ProcessPacket(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(RawPacketLoginSuccess4)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* rawPacket = reinterpret_cast<const RawPacketLoginSuccess4*>(payload.data());

        LoginSuccessData successData;
        successData.sessionKey.handle = rawPacket->handle;
        successData.sessionKey.randomKey = rawPacket->random_key;
        successData.players.reserve(PLAYER_PER_ACCOUNT4_CONST);

        for (size_t i = 0; i < PLAYER_PER_ACCOUNT4_CONST; ++i) {
            const auto& rawPlayer = rawPacket->akSimplePlayerInformation[i];
            
            PlayerSlotData slotData;
            slotData.id = rawPlayer.dwID;
            
            // Bezpieczne kopiowanie ciagu znakow
            slotData.name = std::string(rawPlayer.szName, strnlen(rawPlayer.szName, CHARACTER_NAME_MAX_LEN_CONST));
            
            slotData.job = rawPlayer.byJob;
            slotData.level = rawPlayer.byLevel;
            slotData.playMinutes = rawPlayer.dwPlayMinutes;
            slotData.st = rawPlayer.byST;
            slotData.ht = rawPlayer.byHT;
            slotData.dx = rawPlayer.byDX;
            slotData.iq = rawPlayer.byIQ;
            slotData.mainPart = rawPlayer.wMainPart;
            slotData.changeName = rawPlayer.bChangeName;
            slotData.hairPart = rawPlayer.wHairPart;
            slotData.x = rawPlayer.x;
            slotData.y = rawPlayer.y;
            slotData.addr = rawPlayer.lAddr;
            slotData.port = rawPlayer.wPort;
            slotData.skillGroup = rawPlayer.bySkillGroup;

            slotData.guildId = rawPacket->guild_id[i];
            slotData.guildName = std::string(rawPacket->guild_name[i], strnlen(rawPacket->guild_name[i], GUILD_NAME_MAX_LEN_CONST));

            successData.players.push_back(std::move(slotData));
        }

        if (m_callback) {
            m_callback(successData);
        }

        return {};
    }

} // namespace Client::Network::Handlers
