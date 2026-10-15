#pragma once

#include "Protocol_Common.h"

// ============================================================================
// Subheader enums — Guild, Messenger, Dungeon
// ============================================================================

namespace GuildSub {
    namespace CG { enum : uint8_t {
        ADD_MEMBER,
        REMOVE_MEMBER,
        CHANGE_GRADE_NAME,
        CHANGE_GRADE_AUTHORITY,
        OFFER,
        POST_COMMENT,
        DELETE_COMMENT,
        REFRESH_COMMENT,
        CHANGE_MEMBER_GRADE,
        USE_SKILL,
        CHANGE_MEMBER_GENERAL,
        GUILD_INVITE_ANSWER,
        CHARGE_GSP,
        DEPOSIT_MONEY,
        WITHDRAW_MONEY,
    }; }
    namespace GC { enum : uint8_t {
        LOGIN,
        LOGOUT,
        LIST,
        GRADE,
        ADD,
        REMOVE,
        GRADE_NAME,
        GRADE_AUTH,
        INFO,
        COMMENTS,
        CHANGE_EXP,
        CHANGE_MEMBER_GRADE,
        SKILL_INFO,
        CHANGE_MEMBER_GENERAL,
        GUILD_INVITE,
        WAR,
        GUILD_NAME,
        GUILD_WAR_LIST,
        GUILD_WAR_END_LIST,
        WAR_POINT,
        MONEY_CHANGE,
    }; }
}

namespace MessengerSub {
    namespace CG { enum : uint8_t {
        ADD_BY_VID,
        ADD_BY_NAME,
        REMOVE,
        INVITE_ANSWER,  // Added to match server packet_headers.h
    }; }
    namespace GC { enum : uint8_t {
        LIST,
        LOGIN,
        LOGOUT,
        INVITE,
        REMOVE_FRIEND,
    }; }
}

namespace DungeonSub {
    namespace GC { enum : uint8_t {
        TIME_ATTACK_START = 0,
        DESTINATION_POSITION = 1,
    }; }
}

#pragma pack(push, 1)

// ============================================================================
// Guild Mark & Guild Symbol
// ============================================================================

typedef struct command_mark_login
{
    uint16_t	header;
    uint16_t	length;
    uint32_t   handle;
    uint32_t   random_key;
} TPacketCGMarkLogin;

typedef struct command_mark_upload
{
    uint16_t	header;
    uint16_t	length;
    uint32_t   gid;
    uint8_t    image[16*12*4];
} TPacketCGMarkUpload;

typedef struct command_mark_idxlist
{
    uint16_t	header;
    uint16_t	length;
} TPacketCGMarkIDXList;

typedef struct command_mark_crclist
{
    uint16_t	header;
    uint16_t	length;
    uint8_t    imgIdx;
    uint32_t   crclist[80];
} TPacketCGMarkCRCList;

typedef struct packet_mark_idxlist
{
    uint16_t	header;
    uint16_t	length;
	uint32_t	bufSize;
    uint16_t    count;
    //뒤에 size * (uint16_t + uint16_t)만큼 데이터 붙음
} TPacketGCMarkIDXList;

typedef struct packet_mark_block
{
    uint16_t	header;
    uint16_t	length;
    uint32_t   bufSize;
	uint8_t	imgIdx;
    uint32_t   count;
    // 뒤에 64 x 48 x 픽셀크기(4바이트) = 12288만큼 데이터 붙음
} TPacketGCMarkBlock;

typedef struct packet_mark_update
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	guildID;
	uint16_t	imgIdx;
} TPacketGCMarkUpdate;

typedef struct command_symbol_upload
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	handle;
} TPacketCGSymbolUpload;

typedef struct command_symbol_crc
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwGuildID;
	uint32_t	dwCRC;
	uint32_t	dwSize;
} TPacketCGSymbolCRC;

typedef struct packet_symbol_data
{
    uint16_t	header;
    uint16_t	length;
    uint32_t guild_id;
} TPacketGCGuildSymbolData;

// ============================================================================
// Guild Management, Wars & Territories
// ============================================================================

typedef struct command_guild
{
    uint16_t	header;
    uint16_t	length;
	uint8_t bySubHeader;
} TPacketCGGuild;

typedef struct packet_guild
{
    uint16_t	header;
    uint16_t	length;
    uint8_t subheader;
} TPacketGCGuild;

typedef struct command_guild_answer_make_guild
{
	uint16_t	header;
	uint16_t	length;
	char guild_name[GUILD_NAME_MAX_LEN+1];
} TPacketCGAnswerMakeGuild; 

// SubHeader - Grade
enum
{
    GUILD_AUTH_ADD_MEMBER       = (1 << 0),
    GUILD_AUTH_REMOVE_MEMBER    = (1 << 1),
    GUILD_AUTH_NOTICE           = (1 << 2),
    GUILD_AUTH_SKILL            = (1 << 3),
};

typedef struct packet_guild_sub_grade
{
	char grade_name[GUILD_GRADE_NAME_MAX_LEN+1]; // 8+1 길드장, 길드원 등의 이름
	uint8_t auth_flag;
} TPacketGCGuildSubGrade;

typedef struct packet_guild_sub_member
{
	uint32_t pid;
	uint8_t byGrade;
	uint8_t byIsGeneral;
	uint8_t byJob;
	uint8_t byLevel;
	uint32_t dwOffer;
	uint8_t byNameFlag;
// if NameFlag is TRUE, name is sent from server.
//	char szName[CHARACTER_ME_MAX_LEN+1];
} TPacketGCGuildSubMember;

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

enum EGuildWarState
{
    GUILD_WAR_NONE,
    GUILD_WAR_SEND_DECLARE,
    GUILD_WAR_REFUSE,
    GUILD_WAR_RECV_DECLARE,
    GUILD_WAR_WAIT_START,
    GUILD_WAR_CANCEL,
    GUILD_WAR_ON_WAR,
    GUILD_WAR_END,

    GUILD_WAR_DURATION = 2*60*60, // 2시간
};

typedef struct packet_guild_war
{
    uint32_t       dwGuildSelf;
    uint32_t       dwGuildOpp;
    uint8_t        bType;
    uint8_t        bWarState;
} TPacketGCGuildWar;

typedef struct SPacketGuildWarPoint
{
    uint32_t dwGainGuildID;
    uint32_t dwOpponentGuildID;
    int32_t lPoint;
} TPacketGuildWarPoint;

typedef struct
{
    uint32_t       dwID;
    int32_t        x, y;
    int32_t        width, height;
    uint32_t       dwGuildID;
} TLandPacketElement;

typedef struct packet_land_list
{
    uint16_t	header;
    uint16_t	length;
} TPacketGCLandList;

// ============================================================================
// Party System
// ============================================================================

typedef struct command_party_invite
{
    uint16_t	header;
    uint16_t	length;
    uint32_t vid;
} TPacketCGPartyInvite;

typedef struct command_party_invite_answer
{
    uint16_t	header;
    uint16_t	length;
    uint32_t leader_pid;
    uint8_t accept;
} TPacketCGPartyInviteAnswer;

typedef struct command_party_remove
{
    uint16_t	header;
    uint16_t	length;
    uint32_t pid;
} TPacketCGPartyRemove;

typedef struct command_party_set_state
{
    uint16_t	header;
    uint16_t	length;
    uint32_t dwVID;
	uint8_t byState;
    uint8_t byFlag;
} TPacketCGPartySetState;

typedef struct packet_party_link
{
    uint16_t	header;
    uint16_t	length;
    uint32_t pid;
    uint32_t vid;
} TPacketGCPartyLink;

typedef struct packet_party_unlink
{
    uint16_t	header;
    uint16_t	length;
    uint32_t pid;
	uint32_t vid;
} TPacketGCPartyUnlink;

typedef struct command_party_use_skill
{
    uint16_t	header;
    uint16_t	length;
	uint8_t bySkillIndex;
    uint32_t dwTargetVID;
} TPacketCGPartyUseSkill;

enum EPartyExpDistributionType
{
    PARTY_EXP_DISTRIBUTION_NON_PARITY,
    PARTY_EXP_DISTRIBUTION_PARITY,
};

typedef struct command_party_parameter
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        bDistributeMode;
} TPacketCGPartyParameter;

typedef struct paryt_parameter
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        bDistributeMode;
} TPacketGCPartyParameter;

typedef struct packet_party_invite
{
    uint16_t	header;
    uint16_t	length;
    uint32_t leader_pid;
} TPacketGCPartyInvite;

typedef struct packet_party_add
{
    uint16_t	header;
    uint16_t	length;
    uint32_t pid;
    char name[CHARACTER_NAME_MAX_LEN+1];
} TPacketGCPartyAdd;

typedef struct packet_party_update
{
    uint16_t	header;
    uint16_t	length;
    uint32_t pid;
    uint8_t state;
    uint8_t percent_hp;
    int16_t affects[PARTY_AFFECT_SLOT_MAX_NUM];
} TPacketGCPartyUpdate;

typedef struct packet_party_remove
{
    uint16_t	header;
    uint16_t	length;
    uint32_t pid;
} TPacketGCPartyRemove;

// ============================================================================
// Messenger / Friends System
// ============================================================================

enum
{
	MESSENGER_CONNECTED_STATE_OFFLINE,
	MESSENGER_CONNECTED_STATE_ONLINE,
};

typedef struct packet_messenger
{
    uint16_t	header;
    uint16_t	length;
    uint8_t subheader;
} TPacketGCMessenger;

typedef struct packet_messenger_list_offline
{
    uint8_t connected; // always 0
	uint8_t length;
} TPacketGCMessengerListOffline;

typedef struct packet_messenger_list_online
{
    uint8_t connected;
	uint8_t length;
	//uint8_t length_char_name;
} TPacketGCMessengerListOnline;

typedef struct packet_messenger_login
{
	//uint8_t length_login;
	//uint8_t length_char_name;
	uint8_t length;
} TPacketGCMessengerLogin;

typedef struct packet_messenger_logout
{
	uint8_t length;
} TPacketGCMessengerLogout;

typedef struct command_messenger
{
    uint16_t	header;
    uint16_t	length;
    uint8_t subheader;
} TPacketCGMessenger;

typedef struct command_messenger_remove
{
	uint8_t length;
} TPacketCGMessengerRemove;

enum EBlockAction
{
    BLOCK_EXCHANGE              = (1 << 0),
    BLOCK_PARTY_INVITE          = (1 << 1),
    BLOCK_GUILD_INVITE          = (1 << 2),
    BLOCK_WHISPER               = (1 << 3),
    BLOCK_MESSENGER_INVITE      = (1 << 4),
    BLOCK_PARTY_REQUEST         = (1 << 5),
};

// ============================================================================
// Dungeon System
// ============================================================================

typedef struct command_dungeon
{
	uint16_t	header;
	uint16_t	length;
} TPacketCGDungeon;

typedef struct packet_dungeon
{
	uint16_t	header;
	uint16_t	length;
    uint8_t		subheader;
} TPacketGCDungeon;

#pragma pack(pop)
