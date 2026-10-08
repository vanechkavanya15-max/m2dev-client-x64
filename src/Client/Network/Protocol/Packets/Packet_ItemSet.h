#pragma once

#include <cstdint>
#include "../GameType.h"

#pragma pack(push, 1)

/// @brief Packet structure for GC Item Set
typedef struct packet_set_item {
    uint16_t header;    ///< Packet header
    uint16_t length;    ///< Packet length
    TItemPos pos;       ///< Item position
    uint32_t vnum;      ///< Item vnum
    uint16_t count;     ///< Item count
    uint32_t flags;     ///< Item flags
    union {
        uint32_t antiFlags; ///< Anti flags
        uint32_t anti_flags; ///< Backward compatibility
    };
    uint8_t highlight;  ///< Highlight state
    union {
        int32_t sockets[ITEM_SOCKET_SLOT_MAX_NUM]; ///< Item sockets
        int32_t alSockets[ITEM_SOCKET_SLOT_MAX_NUM]; ///< Backward compatibility
    };
    union {
        TPlayerItemAttribute attributes[ITEM_ATTRIBUTE_SLOT_MAX_NUM]; ///< Item attributes
        TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_SLOT_MAX_NUM]; ///< Backward compatibility
    };
} TPacketGCItemSet;

#pragma pack(pop)
