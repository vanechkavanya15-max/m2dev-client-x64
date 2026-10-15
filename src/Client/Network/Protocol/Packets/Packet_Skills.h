#pragma once

#include <cstdint>

#pragma pack(push, 1)

/**
 * @brief Packet sent by client to request using a skill on a specific target.
 */
struct PacketCGUseSkill
{
    uint16_t header;     ///< Packet header
    uint16_t length;     ///< Packet length
    uint32_t vnum;       ///< Skill VNUM
    uint32_t targetId;   ///< Target Virtual ID (VID)
};

/**
 * @brief Packet sent by client to request using a party skill.
 */
struct PacketCGPartyUseSkill
{
    uint16_t header;       ///< Packet header
    uint16_t length;       ///< Packet length
    uint8_t skillIndex;    ///< Party skill index
    uint32_t targetId;     ///< Target Virtual ID (VID)
};

constexpr uint32_t SKILL_MAX_NUM = 255;

/**
 * @brief Packet sent by server to update simple skill levels.
 */
struct PacketGCSkillLevel
{
    uint16_t header;                            ///< Packet header
    uint16_t length;                            ///< Packet length
    uint8_t skillLevels[SKILL_MAX_NUM];         ///< Array of skill levels
};

/**
 * @brief Data structure holding information about a player's skill.
 */
struct PlayerSkill
{
    uint8_t masterType;  ///< Type of mastery (Normal, Master, Grand Master, Perfect Master)
    uint8_t level;       ///< Current level of the skill
    int64_t nextRead;    ///< Timestamp for next possible book reading
};

/**
 * @brief Packet sent by server to update detailed skill levels (including mastery and read times).
 */
struct PacketGCSkillLevelNew
{
    uint16_t header;                            ///< Packet header
    uint16_t length;                            ///< Packet length
    PlayerSkill skills[SKILL_MAX_NUM];      ///< Array of detailed skill data
};

/**
 * @brief Packet sent by server when a skill's cooldown time ends.
 */
struct PacketGCSkillCoolTimeEnd
{
    uint16_t header;     ///< Packet header
    uint16_t length;     ///< Packet length
    uint8_t skillIndex;  ///< Index of the skill whose cooldown ended
};

/**
 * @brief Packet sent by server to change the player's skill group (e.g., Warrior Body vs Mental).
 */
struct PacketGCChangeSkillGroup
{
    uint16_t header;     ///< Packet header
    uint16_t length;     ///< Packet length
    uint8_t skillGroup;  ///< New skill group index
};

#pragma pack(pop)
