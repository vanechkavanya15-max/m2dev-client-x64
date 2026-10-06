#pragma once

#include <cstdint>
#include <span>

#pragma pack(push, 1)
/**
 * @brief Represents the payload structure for a guild skill info packet.
 * 
 * Ensures strict memory alignment and avoids legacy Hungarian notation types.
 */
struct GuildSkillInfoPacket
{
    uint8_t skillPoint;      ///< Available points to invest in guild skills.
    uint8_t skillLevel[12];  ///< Array storing the level of each guild skill.
    uint16_t guildPoint;     ///< Current guild point (stamina/mana equivalent).
    uint16_t maxGuildPoint;  ///< Maximum capacity of guild points.
};
#pragma pack(pop)

/**
 * @brief Handler for network events related to guild skills.
 *
 * Implements C++20 standard practices without direct UI coupling.
 */
class GuildSkillHandler
{
public:
    /**
     * @brief Parses and applies the skill info payload to the internal memory state.
     * 
     * @param payload Binary buffer containing the incoming data for the packet.
     * @return true If the payload size is valid and data was processed successfully.
     * @return false If the payload size mismatches the expected structure size.
     */
    static bool HandleSkillInfo(std::span<const uint8_t> payload);
};
