#pragma once
#include <cstdint>

namespace CG {
    constexpr uint16_t USE_SKILL = 0x0402;
    constexpr uint16_t SKILL_LEVEL = 0x021A;
    constexpr uint16_t SKILL_LEVEL_NEW = 0x021B;
    constexpr uint16_t SKILL_COOLTIME_END = 0x021C;
}

#define SKILL_MAX_NUM 255

#pragma pack(push, 1)

typedef struct packet_skill_level
{
    uint16_t    header;
    uint16_t    length;
    uint8_t     abSkillLevels[SKILL_MAX_NUM];
} TPacketGCSkillLevel;

typedef struct SPlayerSkill
{
    uint8_t bMasterType;
    uint8_t bLevel;
    int64_t tNextRead; // time_t, force to int64_t to match 2554B for TPacketGCSkillLevelNew
} TPlayerSkill;

typedef struct packet_skill_level_new
{
    uint16_t    header;
    uint16_t    length;
    TPlayerSkill skills[SKILL_MAX_NUM];
} TPacketGCSkillLevelNew;

typedef struct packet_skill_cooltime_end
{
    uint16_t    header;
    uint16_t    length;
    uint8_t     bSkill;
} TPacketGCSkillCoolTimeEnd;

typedef struct command_use_skill
{
    uint16_t    header;
    uint16_t    length;
    uint32_t    dwVnum;
    uint32_t    dwTargetVID;
} TPacketCGUseSkill;

#pragma pack(pop)
