#pragma once

#include <cstdint>
#include <span>

#pragma pack(push, 1)
/**
 * @brief Represents the packet received from the server when a skill cooldown ends.
 */
struct SkillCoolTimeEndPacket
{
    /** @brief Network packet header identifier. */
    uint16_t header;
    /** @brief The length of the packet. */
    uint16_t length;
    /** @brief The ID of the skill whose cooldown has ended. */
    uint8_t skillId;
};
#pragma pack(pop)

/**
 * @brief Handler for processing skill cooldown network events.
 */
class SkillCooldownHandler
{
public:
    /**
     * @brief Parses and handles the skill cooldown end packet.
     * 
     * @param buffer A view over the raw byte buffer received from the network.
     * @return true if the packet was handled successfully, false otherwise.
     */
    static bool HandlePacket(std::span<const uint8_t> buffer);
};
