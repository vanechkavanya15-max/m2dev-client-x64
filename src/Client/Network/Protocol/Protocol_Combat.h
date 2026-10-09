#pragma once

#include "Protocol_Common.h"

#pragma pack(push, 1)

// ============================================================================
// Attack & Battle Modes
// ============================================================================

enum EBattleMode
{
	BATTLEMODE_ATTACK = 0,
	BATTLEMODE_DEFENSE = 1,
};

typedef struct command_attack
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bType;			// 공격 유형
	uint32_t	dwVictimVID;	// 적 VID
	uint8_t	bCRCMagicCubeProcPiece;
	uint8_t	bCRCMagicCubeFilePiece;
} TPacketCGAttack;

typedef struct packet_attack
{
    uint16_t	header;
    uint16_t	length;
    uint32_t       dwVID;
    uint32_t       dwVictimVID;    // 적 VID
    uint8_t        bType;          // 공격 유형
} TPacketGCAttack;

typedef struct packet_damage_info
{
	uint16_t	header;
	uint16_t	length;
	uint32_t dwVID;
	uint8_t flag;
	int32_t  damage;
} TPacketGCDamageInfo;

// ============================================================================
// Combat States, Mounts & Animations
// ============================================================================

typedef struct packet_stun
{
	uint16_t	header;
	uint16_t	length;
	uint32_t		vid;
} TPacketGCStun;

typedef struct packet_dead
{
	uint16_t	header;
	uint16_t	length;
	uint32_t		vid;
} TPacketGCDead;

typedef struct packet_motion
{
	uint16_t	header;
	uint16_t	length;
	uint32_t		vid;
	uint32_t		victim_vid;
	uint16_t		motion;
} TPacketGCMotion;

typedef struct packet_mount
{
    uint16_t	header;
    uint16_t	length;
    uint32_t       vid;
    uint32_t       mount_vid;
    uint8_t        pos;
	uint32_t		_x, _y;
} TPacketGCMount;

typedef struct packet_change_speed
{
	uint16_t	header;
	uint16_t	length;
	uint32_t		vid;
	uint16_t		moving_speed;
} TPacketGCChangeSpeed;

// ============================================================================
// Projectiles & Ranged Targeting
// ============================================================================

typedef struct command_fly_targeting
{
	uint16_t	header;
	uint16_t	length;
	uint32_t		dwTargetVID;
	int32_t		lX;
	int32_t		lY;
} TPacketCGFlyTargeting;

typedef struct packet_fly_targeting
{
    uint16_t	header;
    uint16_t	length;
	uint32_t		dwShooterVID;
	uint32_t		dwTargetVID;
	int32_t		lX;
	int32_t		lY;
} TPacketGCFlyTargeting;

typedef struct packet_shoot
{   
    uint16_t	header;
    uint16_t	length;
    uint8_t		bType;
} TPacketCGShoot;

// fly
typedef struct packet_fly
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        bType;
    uint32_t       dwStartVID;
    uint32_t       dwEndVID;
} TPacketGCCreateFly;

// ============================================================================
// PvP, Duels & PK Modes
// ============================================================================

enum EPKModes
{
	PK_MODE_PEACE,
	PK_MODE_REVENGE,
	PK_MODE_FREE,
	PK_MODE_PROTECT,
	PK_MODE_GUILD,
	PK_MODE_MAX_NUM,
};

enum EPVPModes
{
	PVP_MODE_NONE,
    PVP_MODE_AGREE,
    PVP_MODE_FIGHT,
    PVP_MODE_REVENGE,
};

typedef struct packet_duel_start
{
    uint16_t	header;
    uint16_t	length;
} TPacketGCDuelStart;

typedef struct packet_pvp
{
	uint16_t	header;
	uint16_t	length;
	uint32_t		dwVIDSrc;
	uint32_t		dwVIDDst;
	uint8_t		bMode;
} TPacketGCPVP;

// ============================================================================
// Combat Skills & Cooldowns
// ============================================================================

typedef struct command_use_skill
{
    uint16_t	header;
    uint16_t	length;
    uint32_t               dwVnum;
	uint32_t				dwTargetVID;
} TPacketCGUseSkill;

#ifndef SKILL_MAX_NUM
#define	SKILL_MAX_NUM 255
#endif

typedef struct packet_skill_level
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        abSkillLevels[SKILL_MAX_NUM];
} TPacketGCSkillLevel;

typedef struct SPlayerSkill
{
	uint8_t bMasterType;
	uint8_t bLevel;
	time_t tNextRead;
} TPlayerSkill;

typedef struct packet_skill_level_new
{
	uint16_t	header;
	uint16_t	length;
	TPlayerSkill skills[SKILL_MAX_NUM];
} TPacketGCSkillLevelNew;
static_assert(sizeof(TPacketGCSkillLevelNew) == 2554, "TPacketGCSkillLevelNew must be 2554 bytes on x64");

typedef struct packet_skill_cooltime_end
{
	uint16_t	header;
	uint16_t	length;
	uint8_t		bSkill;
} TPacketGCSkillCoolTimeEnd;

typedef struct SPacketGCChangeSkillGroup
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        skill_group;
} TPacketGCChangeSkillGroup;

// ============================================================================
// Visual Combat & Special Effects
// ============================================================================

enum SPECIAL_EFFECT
{
	SE_NONE,
	SE_HPUP_RED,
	SE_SPUP_BLUE,
	SE_SPEEDUP_GREEN,
	SE_DXUP_PURPLE,
	SE_CRITICAL,
	SE_PENETRATE,
	SE_BLOCK,
	SE_DODGE,
	SE_CHINA_FIREWORK,
	SE_SPIN_TOP,
	SE_SUCCESS,
	SE_FAIL,
	SE_FR_SUCCESS,    
    SE_LEVELUP_ON_14_FOR_GERMANY,	//레벨업 14일때 ( 독일전용 )
    SE_LEVELUP_UNDER_15_FOR_GERMANY,//레벨업 15일때 ( 독일전용 )
    SE_PERCENT_DAMAGE1,
    SE_PERCENT_DAMAGE2,
    SE_PERCENT_DAMAGE3,    
	SE_AUTO_HPUP,
	SE_AUTO_SPUP,
	SE_EQUIP_RAMADAN_RING,			// 초승달의 반지를 착용하는 순간에 발동하는 이펙트
	SE_EQUIP_HALLOWEEN_CANDY,		// 할로윈 사탕을 착용(-_-;)한 순간에 발동하는 이펙트
	SE_EQUIP_HAPPINESS_RING,		// 크리스마스 행복의 반지를 착용하는 순간에 발동하는 이펙트
	SE_EQUIP_LOVE_PENDANT,		// 발렌타인 사랑의 팬던트(71145) 착용할 때 이펙트 (발동이펙트임, 지속이펙트 아님),
	SE_AGGREGATE_MONSTER,
};

typedef struct SPacketGCSpecialEffect
{
    uint16_t	header;
    uint16_t	length;
    uint8_t type;
    uint32_t vid;
} TPacketGCSpecialEffect;

typedef struct SPacketGCSpecificEffect
{
	uint16_t	header;
	uint16_t	length;
	uint32_t vid;
	char effect_file[128];
} TPacketGCSpecificEffect;

#pragma pack(pop)
