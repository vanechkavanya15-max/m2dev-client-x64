#pragma once

#include <cstdint>
#include "../GameType.h"

#pragma pack(push, 1)
/**
 * @brief Represents the request to drop an item with a specific count.
 * 
 * This packet is sent by the client to request dropping a certain amount
 * of an item (with an optional gold parameter depending on context).
 */
struct TPacketCGItemDrop2
{
    /// @brief Network header indicating the packet type.
    uint16_t header;

    /// @brief Length of the packet structure.
    uint16_t length;

    /// @brief The position of the item to drop.
    TItemPos pos;

    /// @brief Gold amount associated with the item drop.
    uint32_t gold;

    /// @brief The number of items to drop.
    uint8_t count;
};
#pragma pack(pop)

