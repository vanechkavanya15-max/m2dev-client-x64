#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <array>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Packet.h"

namespace Network::Dispatchers
{
    /**
     * @brief Event payload for Character Add (Spawn).
     */
    struct CharacterSpawnEvent
    {
        EterBase::EntityId vid;
        float angle;
        int32_t x;
        int32_t y;
        int32_t z;
        uint8_t type;
        uint16_t raceNum;
        uint8_t movingSpeed;
        uint8_t attackSpeed;
        uint8_t stateFlag;
        std::array<uint32_t, 2> affectFlag;
    };

    /**
     * @brief Event payload for Character Add 2 (Spawn with extra details).
     */
    struct CharacterSpawnEvent2
    {
        EterBase::EntityId vid;
        std::string name;
        float angle;
        int32_t x;
        int32_t y;
        int32_t z;
        uint8_t type;
        uint16_t raceNum;
        std::array<uint16_t, CHR_EQUIPPART_NUM> parts;
        uint8_t movingSpeed;
        uint8_t attackSpeed;
        uint8_t stateFlag;
        std::array<uint32_t, 2> affectFlag;
        uint8_t empire;
        EterBase::GuildId guildId;
        int16_t alignment;
        uint8_t pkMode;
        EterBase::ItemVnum mountVnum;
    };

    /**
     * @brief Processes the Character Add packet.
     * 
     * @param buffer The incoming packet buffer.
     * @return EterBase::PacketResult<void> Returns a success result or an error type if processing fails.
     */
    EterBase::PacketResult<void> ProcessCharacterAdd(std::span<const uint8_t> buffer);

    /**
     * @brief Processes the Character Add 2 packet.
     * 
     * @param buffer The incoming packet buffer.
     * @return EterBase::PacketResult<void> Returns a success result or an error type if processing fails.
     */
    EterBase::PacketResult<void> ProcessCharacterAdd2(std::span<const uint8_t> buffer);

} // namespace Network::Dispatchers
