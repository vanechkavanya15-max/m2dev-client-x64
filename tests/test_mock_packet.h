#pragma once
#define ETERBASE_STDAFX_H
#include <cstdint>
struct TItemPos {
    uint8_t window_type;
    uint16_t cell;
};

#define ITEM_SOCKET_SLOT_MAX_NUM 3
#define ITEM_ATTRIBUTE_SLOT_MAX_NUM 7

struct TPlayerItemAttribute {
    uint8_t bType;
    int16_t sValue;
};

namespace ExchangeSub {
    namespace CG { enum : uint8_t { START, ITEM_ADD, ITEM_DEL, ELK_ADD, ACCEPT, CANCEL, }; }
    namespace GC { enum : uint8_t { START, ITEM_ADD, ITEM_DEL, ELK_ADD, ACCEPT, END, ALREADY, LESS_ELK, }; }
}
namespace CG { enum { EXCHANGE = 0x0508 }; }
namespace GC { enum { EXCHANGE = 0x051C }; }

#pragma pack(push, 1)
struct TPacketCGExchange {
	uint16_t header;
	uint16_t length;
	uint8_t subheader;
	uint32_t arg1;
	uint8_t arg2;
	TItemPos Pos;
};

struct TPacketGCExchange {
    uint16_t header;
    uint16_t length;
    uint8_t subheader;
    uint8_t is_me;
    uint32_t arg1;
    TItemPos arg2;
    uint32_t arg3;
	int32_t alValues[ITEM_SOCKET_SLOT_MAX_NUM];
    TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_SLOT_MAX_NUM];
};
#pragma pack(pop)
