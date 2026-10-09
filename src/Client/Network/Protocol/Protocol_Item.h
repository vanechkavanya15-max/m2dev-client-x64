#pragma once

#include "Protocol_Common.h"

// ============================================================================
// Subheader enums — Shop, Exchange, DragonSoul, Fishing
// ============================================================================

namespace ShopSub {
    namespace CG { enum : uint8_t {
        END,
        BUY,
        SELL,
        SELL2,
    }; }
    namespace GC { enum : uint8_t {
        START,
        END,
        UPDATE_ITEM,
        UPDATE_PRICE,
        OK,
        NOT_ENOUGH_MONEY,
        SOLDOUT,
        INVENTORY_FULL,
        INVALID_POS,
        SOLD_OUT,
        START_EX,
        NOT_ENOUGH_MONEY_EX,
    }; }
}

namespace ExchangeSub {
    namespace CG { enum : uint8_t {
        START,
        ITEM_ADD,
        ITEM_DEL,
        ELK_ADD,
        ACCEPT,
        CANCEL,
    }; }
    namespace GC { enum : uint8_t {
        START,
        ITEM_ADD,
        ITEM_DEL,
        ELK_ADD,
        ACCEPT,
        END,
        ALREADY,
        LESS_ELK,
    }; }
}

namespace DragonSoulSub { enum : uint8_t {
    OPEN,
    CLOSE,
    DO_UPGRADE,
    DO_IMPROVEMENT,
    DO_REFINE,
    REFINE_FAIL,
    REFINE_FAIL_MAX_REFINE,
    REFINE_FAIL_INVALID_MATERIAL,
    REFINE_FAIL_NOT_ENOUGH_MONEY,
    REFINE_FAIL_NOT_ENOUGH_MATERIAL,
    REFINE_FAIL_TOO_MUCH_MATERIAL,
    REFINE_SUCCEED,
}; }

namespace FishingSub {
    namespace GC { enum : uint8_t {
        START,
        STOP,
        REACT,
        SUCCESS,
        FAIL,
        FISH,
    }; }
}

#pragma pack(push, 1)

// ============================================================================
// Item Management & Inventory Actions
// ============================================================================

typedef struct command_item_use
{
	uint16_t	header;
	uint16_t	length;
	TItemPos pos;
} TPacketCGItemUse;

typedef struct command_item_use_to_item
{
	uint16_t	header;
	uint16_t	length;
	TItemPos source_pos;
	TItemPos target_pos;
} TPacketCGItemUseToItem;

typedef struct command_item_drop
{
	uint16_t	header;
	uint16_t	length;
	TItemPos pos;
	uint32_t elk;
} TPacketCGItemDrop;

typedef struct command_item_drop2
{
    uint16_t	header;
    uint16_t	length;
    TItemPos pos;
    uint32_t       gold;
    uint8_t        count;
} TPacketCGItemDrop2;

typedef struct command_item_move
{
	uint16_t	header;
	uint16_t	length;
	TItemPos pos;
	TItemPos change_pos;
	uint8_t num;
} TPacketCGItemMove;

typedef struct command_item_pickup
{
	uint16_t	header;
	uint16_t	length;
	uint32_t vid;
} TPacketCGItemPickUp;

typedef struct command_give_item
{
	uint16_t	header;
	uint16_t	length;
	uint32_t dwTargetVID;
	TItemPos ItemPos;
	uint8_t byItemCount;
} TPacketCGGiveItem;

typedef struct packet_del_item
{
	uint16_t	header;
	uint16_t	length;
	TItemPos	pos;
} TPacketGCItemDel;

typedef struct packet_set_item
{
	uint16_t	header;
	uint16_t	length;
	TItemPos				pos;
	uint32_t				vnum;
	uint8_t					count;
	uint32_t				flags;	// 플래그 추가
	uint32_t				anti_flags;	// 플래그 추가
	uint8_t					highlight;
	int32_t					alSockets[ITEM_SOCKET_SLOT_MAX_NUM];
    TPlayerItemAttribute	aAttr[ITEM_ATTRIBUTE_SLOT_MAX_NUM];
} TPacketGCItemSet;
static_assert(sizeof(TPacketGCItemSet) == 54, "TPacketGCItemSet must be 54 bytes on x64");

typedef struct packet_item_get
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwItemVnum;
	uint8_t		bCount;
	uint8_t		bArg;		// 0: normal, 1: from party member
	char		szFromName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGCItemGet;

typedef struct packet_use_item
{
	uint16_t	header;
	uint16_t	length;
	TItemPos	Cell;
	uint32_t		ch_vid;
	uint32_t		victim_vid;

	uint32_t		vnum;
} TPacketGCItemUse;

typedef struct packet_update_item
{
	uint16_t	header;
	uint16_t	length;
	TItemPos	Cell;
	uint8_t		count;
	int32_t		alSockets[ITEM_SOCKET_SLOT_MAX_NUM];
    TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_SLOT_MAX_NUM];
} TPacketGCItemUpdate;

typedef struct packet_ground_add_item
{
    uint16_t	header;
    uint16_t	length;
    int32_t        lX;
	int32_t		lY;
	int32_t		lZ;

    uint32_t       dwVID;
    uint32_t       dwVnum;
} TPacketGCItemGroundAdd;

typedef struct packet_ground_del_item
{
	uint16_t	header;
	uint16_t	length;
	uint32_t		vid;
} TPacketGCItemGroundDel;

typedef struct packet_item_ownership
{
    uint16_t	header;
    uint16_t	length;
    uint32_t       dwVID;
    char        szName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGCItemOwnership;

typedef struct packet_ownership 
{ 
    uint16_t	header;
    uint16_t	length;
    uint32_t               dwOwnerVID; 
    uint32_t               dwVictimVID; 
} TPacketGCOwnership; 

// ============================================================================
// NPC Shops & Player Private Shops
// ============================================================================

typedef struct command_shop
{
	uint16_t	header;
	uint16_t	length;
	uint8_t		subheader;
} TPacketCGShop;

typedef struct packet_shop
{
	uint16_t	header;
	uint16_t	length;
	uint8_t        subheader;
} TPacketGCShop;

typedef struct packet_shop_start
{
	struct packet_shop_item		items[SHOP_HOST_ITEM_MAX_NUM];
} TPacketGCShopStart;

typedef struct packet_shop_start_ex // 다음에 TSubPacketShopTab* shop_tabs 이 따라옴.
{
	typedef struct sub_packet_shop_tab 
	{
		char name[SHOP_TAB_NAME_MAX];
		uint8_t coin_type;
		packet_shop_item items[SHOP_HOST_ITEM_MAX_NUM];
	} TSubPacketShopTab;
	uint32_t owner_vid;
	uint8_t shop_tab_count;
} TPacketGCShopStartEx;

typedef struct packet_shop_update_item
{
	uint8_t						pos;
	struct packet_shop_item		item;
} TPacketGCShopUpdateItem;

typedef struct packet_shop_update_price
{
	int32_t iElkAmount;
} TPacketGCShopUpdatePrice;

// Private Shop
typedef struct SShopItemTable
{
    uint32_t		vnum;
    uint8_t		count;

    TItemPos	pos;			// PC 상점에만 이용
    uint32_t		price;			// PC 상점에만 이용
    uint8_t		display_pos;	//	PC 상점에만 이용, 보일 위치.
} TShopItemTable;

typedef struct SPacketCGMyShop
{
    uint16_t	header;
    uint16_t	length;
    char        szSign[SHOP_SIGN_MAX_LEN + 1];
    uint8_t        bCount;	// count of TShopItemTable, max 39
} TPacketCGMyShop;

typedef struct SPacketGCShopSign
{
    uint16_t	header;
    uint16_t	length;
    uint32_t       dwVID;
    char        szSign[SHOP_SIGN_MAX_LEN + 1];
} TPacketGCShopSign;

// ============================================================================
// Item Exchange
// ============================================================================

typedef struct command_exchange
{
	uint16_t	header;
	uint16_t	length;
	uint8_t		subheader;
	uint32_t		arg1;
	uint8_t		arg2;
	TItemPos	Pos;
} TPacketCGExchange;

typedef struct packet_exchange
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        subheader;
    uint8_t        is_me;
    uint32_t       arg1;
    TItemPos       arg2;
    uint32_t       arg3;
	int32_t		alValues[ITEM_SOCKET_SLOT_MAX_NUM];
    TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_SLOT_MAX_NUM];
} TPacketGCExchange;

// ============================================================================
// Safebox & Item Mall
// ============================================================================

enum
{
	SAFEBOX_MONEY_STATE_SAVE,
	SAFEBOX_MONEY_STATE_WITHDRAW,
};

typedef struct command_safebox_money
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        bState;
    int32_t        lMoney;  // Changed from uint32_t to int32_t to match server packet_structs.h
} TPacketCGSafeboxMoney;

typedef struct command_safebox_checkout
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        bSafePos;
    TItemPos	ItemPos;
} TPacketCGSafeboxCheckout;

typedef struct command_safebox_checkin
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        bSafePos;
    TItemPos	ItemPos;
} TPacketCGSafeboxCheckin;

typedef TPacketCGSafeboxCheckout TPacketGCSafeboxCheckout;
typedef TPacketCGSafeboxCheckin TPacketGCSafeboxCheckin;

typedef struct packet_safebox_wrong_password
{
    uint16_t	header;
    uint16_t	length;
} TPacketGCSafeboxWrongPassword;

typedef struct packet_safebox_size
{
	uint16_t	header;
	uint16_t	length;
	uint8_t bSize;
} TPacketGCSafeboxSize; 

typedef struct packet_safebox_money_change
{
    uint16_t	header;
    uint16_t	length;
    int32_t lMoney;		// Signed to match server (uses int32_t lMoney)
} TPacketGCSafeboxMoneyChange;

typedef struct command_mall_checkout
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        bMallPos;
    TItemPos	ItemPos;
} TPacketCGMallCheckout;

typedef struct packet_mall_open
{
	uint16_t	header;
	uint16_t	length;
	uint8_t bSize;
} TPacketGCMallOpen;

// ============================================================================
// Refine / Upgrading & Dragon Soul
// ============================================================================

typedef struct SPacketCGRefine
{
	uint16_t	header;
	uint16_t	length;
	uint8_t		pos;
	uint8_t		type;
} TPacketCGRefine;

struct TMaterial
{
    uint32_t vnum;
    int32_t  count;
};

typedef struct SRefineTable
{
    uint32_t src_vnum;
    uint32_t result_vnum;
    uint8_t material_count;
    int32_t cost; // 소요 비용
    int32_t prob; // 확률
    TMaterial materials[REFINE_MATERIAL_MAX_NUM];
} TRefineTable;

typedef struct SPacketGCRefineInformation
{
    uint16_t header;
    uint16_t length;
    uint8_t  type;
    uint8_t  pos;
    TRefineTable refine_table;
} TPacketGCRefineInformation;

typedef struct SPacketGCRefineInformationNew
{
	uint16_t	header;
	uint16_t	length;
	uint8_t			type;
	uint8_t			pos;
	TRefineTable	refine_table;
} TPacketGCRefineInformationNew;

// 용혼석
enum EDragonSoulRefineWindowRefineType
{
	DragonSoulRefineWindow_UPGRADE,
	DragonSoulRefineWindow_IMPROVEMENT,
	DragonSoulRefineWindow_REFINE,
};

typedef struct SPacketCGDragonSoulRefine
{
	SPacketCGDragonSoulRefine() : header(CG::DRAGON_SOUL_REFINE), length(sizeof(SPacketCGDragonSoulRefine))
	{}
	uint16_t	header;
	uint16_t	length;
	uint8_t bSubType;
	TItemPos ItemGrid[DS_REFINE_WINDOW_MAX_NUM];
} TPacketCGDragonSoulRefine;

typedef struct SPacketGCDragonSoulRefine
{
	SPacketGCDragonSoulRefine() : header(GC::DRAGON_SOUL_REFINE), length(sizeof(SPacketGCDragonSoulRefine))
	{}
	uint16_t	header;
	uint16_t	length;
	uint8_t bSubType;
	TItemPos Pos;
} TPacketGCDragonSoulRefine;

// ============================================================================
// Equipment View
// ============================================================================

typedef struct SEquipmentItemSet
{
	uint32_t   vnum;
	uint8_t    count;
	int32_t    alSockets[ITEM_SOCKET_SLOT_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_SLOT_MAX_NUM];
} TEquipmentItemSet;

typedef struct pakcet_view_equip
{
    uint16_t	header;
    uint16_t	length;
	uint32_t dwVID;
	TEquipmentItemSet equips[WEAR_MAX_NUM];
} TPacketGCViewEquip;

// ============================================================================
// Fishing & Mining
// ============================================================================

typedef struct command_fishing
{
    uint16_t	header;
    uint16_t	length;
    uint8_t dir;
} TPacketCGFishing;

typedef struct packet_fishing
{
    uint16_t	header;
    uint16_t	length;
    uint8_t subheader;
    uint32_t info;
    uint8_t dir;
} TPacketGCFishing;

typedef struct packet_dig_motion
{
    uint16_t	header;
    uint16_t	length;
    uint32_t vid;
    uint32_t target_vid;
	uint8_t count;
} TPacketGCDigMotion;

#pragma pack(pop)
