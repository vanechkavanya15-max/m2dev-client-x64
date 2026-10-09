#pragma once

#include "Protocol_Common.h"

#pragma pack(push, 1)

// ============================================================================
// Character Selection, Creation, Deletion & Entry
// ============================================================================

typedef struct command_player_select
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	player_index;
} TPacketCGSelectCharacter;

typedef struct command_EnterFrontGame
{
	uint16_t	header;
	uint16_t	length;
} TPacketCGEnterFrontGame;

typedef struct SSimplePlayerInformation
{
    uint32_t               dwID;
    char                szName[CHARACTER_NAME_MAX_LEN + 1];
    uint8_t                byJob;
    uint8_t                byLevel;
    uint32_t               dwPlayMinutes;
    uint8_t                byST, byHT, byDX, byIQ;
//	uint16_t				wParts[CRaceData::PART_MAX_NUM];
    uint16_t                wMainPart;
    uint8_t                bChangeName;
	uint16_t				wHairPart;
    uint8_t                bDummy[4];
	int32_t				x, y;
	uint32_t				lAddr;
	uint16_t				wPort;
	uint8_t				bySkillGroup;
} TSimplePlayerInformation;

typedef struct packet_login_success3
{
	uint16_t	header;
	uint16_t	length;
	TSimplePlayerInformation	akSimplePlayerInformation[PLAYER_PER_ACCOUNT3];
    uint32_t						guild_id[PLAYER_PER_ACCOUNT3];
    char						guild_name[PLAYER_PER_ACCOUNT3][GUILD_NAME_MAX_LEN+1];
	uint32_t handle;
	uint32_t random_key;
} TPacketGCLoginSuccess3;

typedef struct packet_login_success4
{
	uint16_t	header;
	uint16_t	length;
	TSimplePlayerInformation	akSimplePlayerInformation[PLAYER_PER_ACCOUNT4];
    uint32_t						guild_id[PLAYER_PER_ACCOUNT4];
    char						guild_name[PLAYER_PER_ACCOUNT4][GUILD_NAME_MAX_LEN+1];
	uint32_t handle;
	uint32_t random_key;
} TPacketGCLoginSuccess4;

typedef struct command_player_create
{
	uint16_t	header;
	uint16_t	length;
	uint8_t        index;
	char        name[CHARACTER_NAME_MAX_LEN + 1];
	uint16_t        job;
	uint8_t		shape;
	uint8_t		CON;
	uint8_t		INT;
	uint8_t		STR;
	uint8_t		DEX;
} TPacketCGCreateCharacter;

typedef struct command_player_create_success
{
    uint16_t	header;
    uint16_t	length;
    uint8_t						bAccountCharacterSlot;
    TSimplePlayerInformation	kSimplePlayerInfomation;
} TPacketGCPlayerCreateSuccess;

typedef struct command_create_failure
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bType;
} TPacketGCCreateFailure;

typedef struct command_player_delete
{
	uint16_t	header;
	uint16_t	length;
	uint8_t        index;
	char		szPrivateCode[PRIVATE_CODE_LENGTH];
} TPacketCGDestroyCharacter;

typedef struct packet_player_delete_success
{
	uint16_t	header;
	uint16_t	length;
	uint8_t        account_index;
} TPacketGCDestroyCharacterSuccess;

// ============================================================================
// Character Appearance, State & Lifecycle
// ============================================================================

enum
{
	ADD_CHARACTER_STATE_DEAD   = (1 << 0),
	ADD_CHARACTER_STATE_SPAWN  = (1 << 1),
	ADD_CHARACTER_STATE_GUNGON = (1 << 2),
	ADD_CHARACTER_STATE_KILLER = (1 << 3),
	ADD_CHARACTER_STATE_PARTY  = (1 << 4),
};

// 2004.11.20.myevan.CRaceData::PART_MAX_NUM 사용안하게 수정 - 서버에서 사용하는것과 일치하지 않음
enum ECharacterEquipmentPart
{
	CHR_EQUIPPART_ARMOR,
	CHR_EQUIPPART_WEAPON,
	CHR_EQUIPPART_HEAD,
	CHR_EQUIPPART_HAIR,

	CHR_EQUIPPART_NUM,		
};

typedef struct packet_char_additional_info
{
	uint16_t	header;
	uint16_t	length;
	uint32_t   dwVID;
	char    name[CHARACTER_NAME_MAX_LEN + 1];
	uint16_t    awPart[CHR_EQUIPPART_NUM];
	uint8_t	bEmpire;
	uint32_t   dwGuildID;
	uint32_t   dwLevel;
	int16_t   sAlignment; //선악치
	uint8_t    bPKMode;
	uint32_t   dwMountVnum;
} TPacketGCCharacterAdditionalInfo;

typedef struct packet_add_char
{
    uint16_t	header;
    uint16_t	length;

    uint32_t       dwVID;

    //char        name[CHARACTER_NAME_MAX_LEN + 1];

    float       angle;
    int32_t        x;
    int32_t        y;
    int32_t        z;

	uint8_t		bType;
    uint16_t        wRaceNum;
    //uint16_t        awPart[CHR_EQUIPPART_NUM];
    uint8_t        bMovingSpeed;
    uint8_t        bAttackSpeed;

    uint8_t        bStateFlag;
    uint32_t       dwAffectFlag[2];        // ??
    //uint8_t      bEmpire;
    //uint32_t     dwGuild;
    //int16_t     sAlignment;	
	//uint8_t		bPKMode;
	//uint32_t		dwMountVnum;
} TPacketGCCharacterAdd;

typedef struct packet_add_char2
{
    uint16_t	header;
    uint16_t	length;

    uint32_t       dwVID;

    char        name[CHARACTER_NAME_MAX_LEN + 1];

    float       angle;
    int32_t        x;
    int32_t        y;
    int32_t        z;

	uint8_t		bType;
    uint16_t        wRaceNum;
    uint16_t        awPart[CHR_EQUIPPART_NUM];
    uint8_t        bMovingSpeed;
    uint8_t        bAttackSpeed;

    uint8_t        bStateFlag;
    uint32_t       dwAffectFlag[2];        // ??
    uint8_t        bEmpire;

    uint32_t       dwGuild;
    int16_t       sAlignment;
	uint8_t		bPKMode;
	uint32_t		dwMountVnum;
} TPacketGCCharacterAdd2;

typedef struct packet_update_char
{
    uint16_t	header;
    uint16_t	length;
    uint32_t       dwVID;

    uint16_t        awPart[CHR_EQUIPPART_NUM];
    uint8_t        bMovingSpeed;
	uint8_t		bAttackSpeed;

    uint8_t        bStateFlag;
    uint32_t       dwAffectFlag[2];

	uint32_t		dwGuildID;
    int16_t       sAlignment;
	uint8_t		bPKMode;
	uint32_t		dwMountVnum;
} TPacketGCCharacterUpdate;

typedef struct packet_update_char2
{
    uint16_t	header;
    uint16_t	length;
    uint32_t       dwVID;

    uint16_t        awPart[CHR_EQUIPPART_NUM];
    uint8_t        bMovingSpeed;
	uint8_t		bAttackSpeed;

    uint8_t        bStateFlag;
    uint32_t       dwAffectFlag[2];

	uint32_t		dwGuildID;
    int16_t       sAlignment;
	uint8_t		bPKMode;
	uint32_t		dwMountVnum;
} TPacketGCCharacterUpdate2;

typedef struct packet_del_char
{
	uint16_t	header;
	uint16_t	length;
    uint32_t	dwVID;
} TPacketGCCharacterDelete;

typedef struct packet_main_character
{
	enum { MUSIC_NAME_MAX_LEN = 24 };

	uint16_t	header;
	uint16_t	length;
	uint32_t	dwVID;
	uint16_t	wRaceNum;
	char		szName[CHARACTER_NAME_MAX_LEN + 1];
	char		szBGMName[MUSIC_NAME_MAX_LEN + 1];
	float		fBGMVol;
	int32_t		lX, lY, lZ;
	uint8_t		byEmpire;
	uint8_t		bySkillGroup;
} TPacketGCMainCharacter;

// ============================================================================
// Movement, Coordinate Replication & Warping
// ============================================================================

typedef struct command_position
{   
    uint16_t	header;
    uint16_t	length;
    uint8_t        position;
} TPacketCGPosition;

typedef struct packet_position
{
    uint16_t	header;
    uint16_t	length;
	uint32_t		vid;
    uint8_t        position;
} TPacketGCPosition;

typedef struct command_move
{
	uint16_t	header;
	uint16_t	length;
	uint8_t		bFunc;
	uint8_t		bArg;
	uint8_t		bRot;
	int32_t		lX;		// Signed to match server (can be negative coordinates)
	int32_t		lY;		// Signed to match server
	uint32_t	dwTime;
} TPacketCGMove;

typedef struct packet_move
{	
	uint16_t	header;
	uint16_t	length;
	uint8_t		bFunc;
	uint8_t		bArg;
	uint8_t		bRot;
	uint32_t		dwVID;
	int32_t		lX;
	int32_t		lY;
	uint32_t		dwTime;
	uint32_t		dwDuration;
} TPacketGCMove;

typedef struct command_sync_position_element 
{ 
    uint32_t       dwVID; 
    int32_t        lX; 
    int32_t        lY; 
} TPacketCGSyncPositionElement; 

typedef struct command_sync_position
{ 
    uint16_t	header;
    uint16_t	length;
} TPacketCGSyncPosition; 

typedef struct packetd_sync_position_element 
{ 
    uint32_t       dwVID; 
    int32_t        lX; 
    int32_t        lY; 
} TPacketGCSyncPositionElement; 

typedef struct packetd_sync_position
{ 
    uint16_t	header;
    uint16_t	length;
} TPacketGCSyncPosition; 

typedef struct command_warp
{
	uint16_t	header;
	uint16_t	length;
} TPacketCGWarp;

typedef struct packet_warp
{
	uint16_t	header;
	uint16_t	length;
	int32_t			lX;
	int32_t			lY;
	int32_t			lAddr;
	uint16_t			wPort;
} TPacketGCWarp;

enum
{
    WALKMODE_RUN,
    WALKMODE_WALK,
};

typedef struct SPacketGCWalkMode
{
    uint16_t	header;
    uint16_t	length;
    uint32_t       vid;
    uint8_t        mode;
} TPacketGCWalkMode;

// ============================================================================
// Character Points, Stats & Attributes
// ============================================================================

enum EPointTypes
{
    POINT_NONE,                 // 0
    POINT_LEVEL,                // 1
    POINT_VOICE,                // 2
    POINT_EXP,                  // 3
    POINT_NEXT_EXP,             // 4
    POINT_HP,                   // 5
    POINT_MAX_HP,               // 6
    POINT_SP,                   // 7
    POINT_MAX_SP,               // 8  
    POINT_STAMINA,              // 9  스테미너
    POINT_MAX_STAMINA,          // 10 최대 스테미너
    
    POINT_GOLD,                 // 11
    POINT_ST,                   // 12 근력
    POINT_HT,                   // 13 체력
    POINT_DX,                   // 14 민첩성
    POINT_IQ,                   // 15 정신력
    POINT_ATT_POWER,            // 16 공격력
    POINT_ATT_SPEED,            // 17 공격속도
    POINT_EVADE_RATE,           // 18 회피율
    POINT_MOV_SPEED,            // 19 이동속도
    POINT_DEF_GRADE,            // 20 방어등급
	POINT_CASTING_SPEED,        // 21 주문속도 (쿨다운타임*100) / (100 + 이값) = 최종 쿨다운 타임
	POINT_MAGIC_ATT_GRADE,      // 22 마법공격력
    POINT_MAGIC_DEF_GRADE,      // 23 마법방어력
    POINT_EMPIRE_POINT,         // 24 제국점수
    POINT_LEVEL_STEP,           // 25 한 레벨에서의 단계.. (1 2 3 될 때 보상, 4 되면 레벨 업)
    POINT_STAT,                 // 26 능력치 올릴 수 있는 개수
	POINT_SUB_SKILL,            // 27 보조 스킬 포인트
	POINT_SKILL,                // 28 액티브 스킬 포인트
//    POINT_SKILL_PASV,           // 27 패시브 기술 올릴 수 있는 개수
//    POINT_SKILL_ACTIVE,         // 28 액티브 스킬 포인트
	POINT_MIN_ATK,				// 29 최소 파괴력
	POINT_MAX_ATK,				// 30 최대 파괴력
    POINT_PLAYTIME,             // 31 플레이시간
    POINT_HP_REGEN,             // 32 HP 회복률
    POINT_SP_REGEN,             // 33 SP 회복률
    
    POINT_BOW_DISTANCE,         // 34 활 사정거리 증가치 (meter)
    
    POINT_HP_RECOVERY,          // 35 체력 회복 증가량
    POINT_SP_RECOVERY,          // 36 정신력 회복 증가량
    
    POINT_POISON_PCT,           // 37 독 확률
    POINT_STUN_PCT,             // 38 기절 확률
    POINT_SLOW_PCT,             // 39 슬로우 확률
    POINT_CRITICAL_PCT,         // 40 크리티컬 확률
    POINT_PENETRATE_PCT,        // 41 관통타격 확률
    POINT_CURSE_PCT,            // 42 저주 확률
    
    POINT_ATTBONUS_HUMAN,       // 43 인간에게 강함
    POINT_ATTBONUS_ANIMAL,      // 44 동물에게 데미지 % 증가
    POINT_ATTBONUS_ORC,         // 45 웅귀에게 데미지 % 증가
    POINT_ATTBONUS_MILGYO,      // 46 밀교에게 데미지 % 증가
    POINT_ATTBONUS_UNDEAD,      // 47 시체에게 데미지 % 증가
    POINT_ATTBONUS_DEVIL,       // 48 마귀(악마)에게 데미지 % 증가
    POINT_ATTBONUS_INSECT,      // 49 벌레족
    POINT_ATTBONUS_FIRE,        // 50 화염족
    POINT_ATTBONUS_ICE,         // 51 빙설족
    POINT_ATTBONUS_DESERT,      // 52 사막족
    POINT_ATTBONUS_UNUSED0,     // 53 UNUSED0
    POINT_ATTBONUS_UNUSED1,     // 54 UNUSED1
    POINT_ATTBONUS_UNUSED2,     // 55 UNUSED2
    POINT_ATTBONUS_UNUSED3,     // 56 UNUSED3
    POINT_ATTBONUS_UNUSED4,     // 57 UNUSED4
    POINT_ATTBONUS_UNUSED5,     // 58 UNUSED5
    POINT_ATTBONUS_UNUSED6,     // 59 UNUSED6
    POINT_ATTBONUS_UNUSED7,     // 60 UNUSED7
    POINT_ATTBONUS_UNUSED8,     // 61 UNUSED8
    POINT_ATTBONUS_UNUSED9,     // 62 UNUSED9

    POINT_STEAL_HP,             // 63 생명력 흡수
    POINT_STEAL_SP,             // 64 정신력 흡수

    POINT_MANA_BURN_PCT,        // 65 마나 번

    /// 피해시 보너스 ///

    POINT_DAMAGE_SP_RECOVER,    // 66 공격당할 시 정신력 회복 확률

    POINT_BLOCK,                // 67 블럭율
    POINT_DODGE,                // 68 회피율

    POINT_RESIST_SWORD,         // 69
    POINT_RESIST_TWOHAND,       // 70
    POINT_RESIST_DAGGER,        // 71
    POINT_RESIST_BELL,          // 72
    POINT_RESIST_FAN,           // 73
    POINT_RESIST_BOW,           // 74  화살   저항   : 대미지 감소
    POINT_RESIST_FIRE,          // 75  화염   저항   : 화염공격에 대한 대미지 감소
    POINT_RESIST_ELEC,          // 76  전기   저항   : 전기공격에 대한 대미지 감소
    POINT_RESIST_MAGIC,         // 77  술법   저항   : 모든술법에 대한 대미지 감소
    POINT_RESIST_WIND,          // 78  바람   저항   : 바람공격에 대한 대미지 감소

    POINT_REFLECT_MELEE,        // 79 공격 반사

    /// 특수 피해시 ///
    POINT_REFLECT_CURSE,        // 80 저주 반사
    POINT_POISON_REDUCE,        // 81 독데미지 감소

    /// 적 소멸시 ///
    POINT_KILL_SP_RECOVER,      // 82 적 소멸시 MP 회복
    POINT_EXP_DOUBLE_BONUS,     // 83
    POINT_GOLD_DOUBLE_BONUS,    // 84
    POINT_ITEM_DROP_BONUS,      // 85

    /// 회복 관련 ///
    POINT_POTION_BONUS,         // 86
    POINT_KILL_HP_RECOVER,      // 87

    POINT_IMMUNE_STUN,          // 88
    POINT_IMMUNE_SLOW,          // 89
    POINT_IMMUNE_FALL,          // 90
    //////////////////

    POINT_PARTY_ATT_GRADE,      // 91
    POINT_PARTY_DEF_GRADE,      // 92

    POINT_ATT_BONUS,            // 93
    POINT_DEF_BONUS,            // 94

    POINT_ATT_GRADE_BONUS,			// 95
    POINT_DEF_GRADE_BONUS,			// 96
    POINT_MAGIC_ATT_GRADE_BONUS,	// 97
    POINT_MAGIC_DEF_GRADE_BONUS,	// 98

    POINT_RESIST_NORMAL_DAMAGE,		// 99

	// MR-10: Added missing POINT_* values
	POINT_HIT_HP_RECOVERY,		// 100
	POINT_HIT_SP_RECOVERY, 		// 101
	POINT_MANASHIELD,			// 102 흑신수호 스킬에 의한 마나쉴드 효과 정도

	POINT_PARTY_BUFFER_BONUS,		// 103
	POINT_PARTY_SKILL_MASTER_BONUS,	// 104

	POINT_HP_RECOVER_CONTINUE,		// 105
	POINT_SP_RECOVER_CONTINUE,		// 106

	POINT_STEAL_GOLD,			// 107 
	POINT_POLYMORPH,			// 108 변신한 몬스터 번호
	POINT_MOUNT,			// 109 타고있는 몬스터 번호

	POINT_PARTY_HASTE_BONUS,		// 110
	POINT_PARTY_DEFENDER_BONUS,		// 111
	// MR-10: -- END OF -- Added missing POINT_* values

	POINT_STAT_RESET_COUNT = 112,
    POINT_HORSE_SKILL = 113,

	POINT_MALL_ATTBONUS,		// 114 공격력 +x%
	POINT_MALL_DEFBONUS,		// 115 방어력 +x%
	POINT_MALL_EXPBONUS,		// 116 경험치 +x%
	POINT_MALL_ITEMBONUS,		// 117 아이템 드롭율 x/10배
	POINT_MALL_GOLDBONUS,		// 118 돈 드롭율 x/10배
    POINT_MAX_HP_PCT,			// 119 최대생명력 +x%
    POINT_MAX_SP_PCT,			// 120 최대정신력 +x%

	POINT_SKILL_DAMAGE_BONUS,       // 121 스킬 데미지 *(100+x)%
	POINT_NORMAL_HIT_DAMAGE_BONUS,  // 122 평타 데미지 *(100+x)%
   
    POINT_SKILL_DEFEND_BONUS,       // 123 스킬 방어 데미지
    POINT_NORMAL_HIT_DEFEND_BONUS,  // 124 평타 방어 데미지
    POINT_PC_BANG_EXP_BONUS,        // 125
	POINT_PC_BANG_DROP_BONUS,       // 126 PC방 전용 드롭률 보너스
	// MR-10: Added missing POINT_* values
	POINT_RAMADAN_CANDY_BONUS_EXP,			// 라마단 사탕 경험치 증가용
	// MR-10: -- END OF -- Added missing POINT_* values

	POINT_ENERGY = 128,				// 128 기력

	// 기력 ui 용.
	// 이렇게 하고 싶지 않았지만, 
	// uiTaskBar에서는 affect에 접근할 수 없고,
	// 더구나 클라리언트에서는 blend_affect는 관리하지 않아,
	// 임시로 이렇게 둔다.
	POINT_ENERGY_END_TIME = 129,	// 129 기력 종료 시간

	// MR-10: Added missing POINT_* values
	POINT_COSTUME_ATTR_BONUS = 130,
	POINT_MAGIC_ATT_BONUS_PER = 131,
	POINT_MELEE_MAGIC_ATT_BONUS_PER = 132,

	// 추가 속성 저항
	POINT_RESIST_ICE = 133,          //   냉기 저항   : 얼음공격에 대한 대미지 감소
	POINT_RESIST_EARTH = 134,        //   대지 저항   : 얼음공격에 대한 대미지 감소
	POINT_RESIST_DARK = 135,         //   어둠 저항   : 얼음공격에 대한 대미지 감소

	POINT_RESIST_CRITICAL = 136,		// 크리티컬 저항	: 상대의 크리티컬 확률을 감소
	POINT_RESIST_PENETRATE = 137,		// 관통타격 저항	: 상대의 관통타격 확률을 감소
	// MR-10: -- END OF -- Added missing POINT_* values

	// 클라이언트 포인트
	POINT_MIN_WEP = 200,
	POINT_MAX_WEP,
	POINT_MIN_MAGIC_WEP,
	POINT_MAX_MAGIC_WEP,
	POINT_HIT_RATE,


    //POINT_MAX_NUM = 255,=>stdafx.h 로/
};

typedef struct packet_points
{
    uint16_t	header;
    uint16_t	length;
    int32_t        points[POINT_MAX_NUM];
} TPacketGCPoints;

typedef struct packet_point_change
{
    uint16_t	header;
    uint16_t	length;

	uint32_t		dwVID;
	uint8_t		Type;

	int32_t        amount; // 바뀐 값
    int32_t        value;  // 현재 값
} TPacketGCPointChange;

// ============================================================================
// Quickslots
// ============================================================================

typedef struct command_quickslot_add
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        pos;
	TQuickSlot	slot;
} TPacketCGQuickSlotAdd;

typedef struct command_quickslot_del
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        pos;
} TPacketCGQuickSlotDel;

typedef struct command_quickslot_swap
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        pos;
    uint8_t        change_pos;
} TPacketCGQuickSlotSwap;

typedef struct packet_quickslot_add
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        pos;
	TQuickSlot	slot;
} TPacketGCQuickSlotAdd;

typedef struct packet_quickslot_del
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        pos;
} TPacketGCQuickSlotDel;

typedef struct packet_quickslot_swap
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        pos;
    uint8_t        change_pos;
} TPacketGCQuickSlotSwap;

// ============================================================================
// Chat & Messaging
// ============================================================================

enum EChatType
{
	CHAT_TYPE_TALKING,  /* 그냥 채팅 */
	CHAT_TYPE_INFO,     /* 정보 (아이템을 집었다, 경험치를 얻었다. 등) */
	CHAT_TYPE_NOTICE,   /* 공지사항 */
	CHAT_TYPE_PARTY,    /* 파티말 */
	CHAT_TYPE_GUILD,    /* 길드말 */
	CHAT_TYPE_COMMAND,	/* 명령 */
	CHAT_TYPE_SHOUT,	/* 외치기 */
	CHAT_TYPE_WHISPER,	// 서버와는 연동되지 않는 Only Client Enum
	CHAT_TYPE_BIG_NOTICE,
	CHAT_TYPE_MAX_NUM,
};

typedef struct command_chat
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	type;
} TPacketCGChat;

typedef struct packet_chatting
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	type;
	uint32_t	dwVID;
	uint8_t	bEmpire;
} TPacketGCChat;

typedef struct command_whisper
{
    uint16_t	header;
    uint16_t	length;
    char        szNameTo[CHARACTER_NAME_MAX_LEN + 1];
} TPacketCGWhisper;

typedef struct packet_whisper   // 가변 패킷    
{   
    uint16_t	header;
    uint16_t	length;
    uint8_t        bType;
    char        szNameFrom[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGCWhisper;

// ============================================================================
// Quests & NPC Dialog Scripts
// ============================================================================

typedef struct command_script_answer
{
    uint16_t	header;
    uint16_t	length;
	uint8_t		answer;
} TPacketCGScriptAnswer;

typedef struct command_script_button
{
    uint16_t	header;
    uint16_t	length;
	uint32_t		idx;
} TPacketCGScriptButton;

typedef struct command_script_select_item
{
    uint16_t	header;
    uint16_t	length;
    uint32_t selection;
} TPacketCGScriptSelectItem;

typedef struct packet_script
{
    uint16_t	header;
    uint16_t	length;
	uint8_t		skin;
    uint16_t        src_size;
} TPacketGCScript;

typedef struct command_quest_input_string
{
    uint16_t	header;
    uint16_t	length;
    char		szString[QUEST_INPUT_STRING_MAX_NUM+1];
} TPacketCGQuestInputString;

typedef struct command_quest_confirm
{
    uint16_t	header;
    uint16_t	length;
    uint8_t answer;
    uint32_t requestPID;
} TPacketCGQuestConfirm;

typedef struct command_quest_cancel
{
    uint16_t	header;
    uint16_t	length;
} TPacketCGQuestCancel;

enum
{
	QUEST_SEND_IS_BEGIN         = 1 << 0,
    QUEST_SEND_TITLE            = 1 << 1,  // 28자 까지
    QUEST_SEND_CLOCK_NAME       = 1 << 2,  // 16자 까지
    QUEST_SEND_CLOCK_VALUE      = 1 << 3,
    QUEST_SEND_COUNTER_NAME     = 1 << 4,  // 16자 까지
    QUEST_SEND_COUNTER_VALUE    = 1 << 5,
	QUEST_SEND_ICON_FILE		= 1 << 6,  // 24자 까지 
};

typedef struct packet_quest_info
{
	uint16_t	header;
	uint16_t	length;
	uint16_t index;
	uint8_t flag;
} TPacketGCQuestInfo;

typedef struct packet_quest_confirm
{
    uint16_t	header;
    uint16_t	length;
    char msg[64+1];
    int32_t timeout;
    uint32_t requestPID;
} TPacketGCQuestConfirm;

// ============================================================================
// Target Tracking & NPC Positions
// ============================================================================

typedef struct command_target
{
    uint16_t	header;
    uint16_t	length;
    uint32_t       dwVID;
} TPacketCGTarget;

typedef struct packet_target
{
    uint16_t	header;
    uint16_t	length;
    uint32_t       dwVID;
    uint8_t        bHPPercent;
} TPacketGCTarget;

typedef struct
{
    uint16_t	header;
    uint16_t	length;
    int32_t        lID;
    char        szTargetName[32+1];
} TPacketGCTargetCreate;

enum
{
	CREATE_TARGET_TYPE_NONE,
	CREATE_TARGET_TYPE_LOCATION,
	CREATE_TARGET_TYPE_CHARACTER,
};

typedef struct
{
	uint16_t	header;
	uint16_t	length;
	int32_t		lID;
	char		szTargetName[32+1];
	uint32_t		dwVID;
	uint8_t		byType;
} TPacketGCTargetCreateNew;

typedef struct
{
    uint16_t	header;
    uint16_t	length;
    int32_t        lID;
    int32_t        lX, lY;
} TPacketGCTargetUpdate;

typedef struct
{
    uint16_t	header;
    uint16_t	length;
    int32_t        lID;
} TPacketGCTargetDelete;

struct TNPCPosition
{
    uint8_t bType;
    uint32_t dwVnum;
    char name[CHARACTER_NAME_MAX_LEN+1];
    int32_t x;
    int32_t y;
};

typedef struct SPacketGCNPCPosition
{
    uint16_t	header;
    uint16_t	length;
    uint16_t count;
} TPacketGCNPCPosition;

typedef struct command_on_click
{
	uint16_t	header;
	uint16_t	length;
	uint32_t		vid;
} TPacketCGOnClick;

// ============================================================================
// Affects & Buffs
// ============================================================================

typedef struct
{
    uint32_t       dwType;
    uint8_t        bPointIdxApplyOn;
    int32_t        lApplyValue;
    uint32_t       dwFlag;
    int32_t        lDuration;
    int32_t        lSPCost;
} TPacketAffectElement;

typedef struct 
{
    uint16_t	header;
    uint16_t	length;
    TPacketAffectElement elem;
} TPacketGCAffectAdd;

typedef struct
{
    uint16_t	header;
    uint16_t	length;
    uint32_t dwType;
    uint8_t bApplyOn;
} TPacketGCAffectRemove;

// ============================================================================
// Character Name Changes & Social Relations
// ============================================================================

typedef struct SPacketCGChangeName
{
    uint16_t	header;
    uint16_t	length;
    uint8_t index;
    char name[CHARACTER_NAME_MAX_LEN+1];
} TPacketCGChangeName;

typedef struct SPacketGCChangeName
{
    uint16_t	header;
    uint16_t	length;
    uint32_t pid;
    char name[CHARACTER_NAME_MAX_LEN+1];
} TPacketGCChangeName;

typedef struct packet_lover_info
{
	uint16_t	header;
	uint16_t	length;
	char szName[CHARACTER_NAME_MAX_LEN + 1];
	uint8_t byLovePoint;
} TPacketGCLoverInfo;

typedef struct packet_love_point_update
{
	uint16_t	header;
	uint16_t	length;
	uint8_t byLovePoint;
} TPacketGCLovePointUpdate;

#pragma pack(pop)
