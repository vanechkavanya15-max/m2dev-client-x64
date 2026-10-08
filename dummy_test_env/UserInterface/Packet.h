#pragma once
#include <cstdint>

#define GUILD_GRADE_NAME_MAX_LEN 8
#define GUILD_NAME_MAX_LEN 12

#pragma pack(push, 1)

typedef struct packet_guild
{
    uint16_t	header;
    uint16_t	length;
    uint8_t subheader;
} TPacketGCGuild;

typedef struct packet_guild_sub_info
{
    uint16_t member_count;
    uint16_t max_member_count;
	uint32_t guild_id;
    uint32_t master_pid;
    uint32_t exp;
    uint8_t level;
    char name[GUILD_NAME_MAX_LEN+1];
	uint32_t gold;
	uint8_t hasLand;
} TPacketGCGuildInfo;

typedef struct packet_guild_sub_member
{
	uint32_t pid;
	uint8_t byGrade;
	uint8_t byIsGeneral;
	uint8_t byJob;
	uint8_t byLevel;
	uint32_t dwOffer;
	uint8_t byNameFlag;
} TPacketGCGuildSubMember;

typedef struct packet_guild_war
{
    uint32_t       dwGuildSelf;
    uint32_t       dwGuildOpp;
    uint8_t        bType;
    uint8_t        bWarState;
} TPacketGCGuildWar;

typedef struct command_guild
{
    uint16_t	header;
    uint16_t	length;
	uint8_t bySubHeader;
} TPacketCGGuild;

#pragma pack(pop)

namespace CG {
    constexpr uint16_t GUILD = 0x0720;
}

namespace GuildSub {
    namespace CG { enum : uint8_t {
        ADD_MEMBER = 0,
        REMOVE_MEMBER = 1,
    }; }
}

