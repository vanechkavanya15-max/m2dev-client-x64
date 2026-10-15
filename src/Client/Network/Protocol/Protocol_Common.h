#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <ctime>

#include "ProtocolTypes.h"
#include "ProtocolOpcodes.h"
#include "EterLib/ControlPackets.h"

// ============================================================================
// Phase constants
// ============================================================================

enum EPhases
{
	PHASE_CLOSE,
	PHASE_HANDSHAKE,
	PHASE_LOGIN,
	PHASE_SELECT,
	PHASE_LOADING,
	PHASE_GAME,
	PHASE_DEAD,

	PHASE_CLIENT_CONNECTING,
	PHASE_DBCLIENT,
	PHASE_P2P,
	PHASE_AUTH,
};

// ============================================================================
// Common size limits & protocol constants
// ============================================================================

enum
{
	ID_MAX_NUM = 30,
	PASS_MAX_NUM = 16,
	CHAT_MAX_NUM = 128,
	PATH_NODE_MAX_NUM = 64,
	SHOP_SIGN_MAX_LEN = 32,

	PLAYER_PER_ACCOUNT3 = 3,
	PLAYER_PER_ACCOUNT4 = 4,

	PLAYER_ITEM_SLOT_MAX_NUM = 20,		// 플래이어의 슬롯당 들어가는 갯수.

	QUICKSLOT_MAX_LINE = 4,
	QUICKSLOT_MAX_COUNT_PER_LINE = 8, // 클라이언트 임의 결정값
	QUICKSLOT_MAX_COUNT = QUICKSLOT_MAX_LINE * QUICKSLOT_MAX_COUNT_PER_LINE,

	QUICKSLOT_MAX_NUM = 36, // 서버와 맞춰져 있는 값

	SHOP_HOST_ITEM_MAX_NUM = 40,

	METIN_SOCKET_COUNT = 6,

	PARTY_AFFECT_SLOT_MAX_NUM = 7,

	GUILD_GRADE_NAME_MAX_LEN = 8,
	GUILD_NAME_MAX_LEN = 12,
	GUILD_GRADE_COUNT = 15,
	GULID_COMMENT_MAX_LEN = 50,

	MARK_CRC_NUM = 8*8,
	MARK_DATA_SIZE = 16*12,
	SYMBOL_DATA_SIZE = 128*256,
	QUEST_INPUT_STRING_MAX_NUM = 64,

	PRIVATE_CODE_LENGTH = 8,

	REFINE_MATERIAL_MAX_NUM = 5,

	WEAR_MAX_NUM = 11,

	SHOP_TAB_NAME_MAX = 32,
	SHOP_TAB_COUNT_MAX = 3,
};

#pragma pack(push, 1)

// ============================================================================
// Handshake, Authentication & Login Packets
// ============================================================================

typedef struct command_checkin
{
	uint16_t	header;
	uint16_t	length;
	char name[ID_MAX_NUM+1];
	char pwd[PASS_MAX_NUM+1];
} TPacketCGCheckin;

// start - 권한 서버 접속을 위한 패킷들
typedef struct command_login2
{
	uint16_t	header;
	uint16_t	length;
	char	name[ID_MAX_NUM + 1];
	uint32_t	login_key;
} TPacketCGLogin2;

typedef struct command_login3
{
    uint16_t	header;
    uint16_t	length;
    char	name[ID_MAX_NUM + 1];
    char	pwd[PASS_MAX_NUM + 1];
} TPacketCGLogin3;

typedef struct command_direct_enter
{
    uint16_t	header;
    uint16_t	length;
    char        login[ID_MAX_NUM + 1];
    char        passwd[PASS_MAX_NUM + 1];
    uint8_t        index;
} TPacketCGDirectEnter;

typedef struct packet_login_key
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwLoginKey;
} TPacketGCLoginKey;

typedef struct packet_auth_success
{
    uint16_t	header;
    uint16_t	length;
    uint32_t       dwLoginKey;
    uint8_t        bResult;
} TPacketGCAuthSuccess;

enum { LOGIN_STATUS_MAX_LEN = 8 };
typedef struct packet_login_failure
{
	uint16_t	header;
	uint16_t	length;
	char	szStatus[LOGIN_STATUS_MAX_LEN + 1];
} TPacketGCLoginFailure;

typedef struct packet_channel
{
    uint16_t	header;
    uint16_t	length;
    uint8_t channel;
} TPacketGCChannel;

typedef struct SChannelStatus
{
	int16_t nPort;
	uint8_t bStatus;
} TChannelStatus;

// Secure authentication packets (libsodium/XChaCha20-Poly1305)
// Client -> Server: Secure login
struct TPacketCGLoginSecure
{
	uint16_t	header; // CG::LOGIN_SECURE
	uint16_t	length;
	char name[ID_MAX_NUM + 1];
	char pwd[PASS_MAX_NUM + 1];
	uint8_t session_token[32]; // Session token from KeyComplete
};

// ============================================================================
// Generic Framing & Time Packets
// ============================================================================

typedef struct packet_blank
{
	uint16_t	header;
	uint16_t	length;
} TPacketGCBlank;

typedef TPacketGCBlank TPacketGCBlankDynamic;

typedef struct packet_header_dynamic_size
{
	uint16_t	header;
	uint16_t	length;
} TDynamicSizePacketHeader;

typedef struct packet_GlobalTime
{
	uint16_t	header;
	uint16_t	length;
	float	GlobalTime;
} TPacketGCGlobalTime;

typedef struct SPacketGCTime
{
    uint16_t	header;
    uint16_t	length;
    time_t      time;
} TPacketGCTime;
static_assert(sizeof(TPacketGCTime) == 12, "TPacketGCTime must be 12 bytes on x64");

typedef struct SPacketGCOnTime
{
    uint16_t	header;
    uint16_t	length;
    int32_t ontime;     // sec
} TPacketGCOnTime;

typedef struct SPacketGCResetOnTime
{
    uint16_t	header;
    uint16_t	length;
} TPacketGCResetOnTime;

// ============================================================================
// Version, Integrity & Anti-Cheat Packets
// ============================================================================

typedef struct command_client_version
{
	uint16_t	header;
	uint16_t	length;
	char filename[32+1];
	char timestamp[32+1];
} TPacketCGClientVersion;

typedef struct command_crc_report
{
	uint16_t	header;
	uint16_t	length;
	uint8_t byPackMode;
	uint32_t dwBinaryCRC32;
	uint32_t dwProcessCRC32;
	uint32_t dwRootPackCRC32;
} TPacketCGCRCReport;

typedef struct SPacketCGHack
{
    uint16_t	header;
    uint16_t	length;
    char        szBuf[255 + 1];
} TPacketCGHack;

// AUTOBAN
typedef struct packet_autoban_quiz
{
    uint16_t	header;
    uint16_t	length;
	uint8_t bDuration;
    uint8_t bCaptcha[64*32];
    char szQuiz[256];
} TPacketGCAutoBanQuiz;
// END_OF_AUTOBAN

// ============================================================================
// Empire Selection Packets
// ============================================================================

typedef struct command_empire
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        bEmpire;
} TPacketCGEmpire;

typedef struct packet_empire
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        bEmpire;
} TPacketGCEmpire;

// ============================================================================
// Client To Client & State Replication
// ============================================================================

typedef struct packet_c2c
{
	uint16_t	header;
	uint16_t	length;
} TPacketGCC2C;

typedef struct packet_state
{
	uint16_t	header;
	uint16_t	length;
	uint8_t			bFunc;
	uint8_t			bArg;
	uint8_t			bRot;
	uint32_t			dwVID;
	uint32_t			dwTime;
	TPixelPosition	kPPos;
} TPacketCCState;

// ============================================================================
// Observer Packets
// ============================================================================

typedef struct packet_observer_add
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	vid;
	uint16_t	x;
	uint16_t	y;
} TPacketGCObserverAdd;

typedef struct packet_observer_move
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	vid;
	uint16_t	x;
	uint16_t	y;
} TPacketGCObserverMove;

typedef struct packet_observer_remove
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	vid;	
} TPacketGCObserverRemove;

#pragma pack(pop)
