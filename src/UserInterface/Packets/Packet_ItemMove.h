#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include "../GameType.h" // For TItemPos

/**
 * @brief Packets related to item move
 */

#pragma pack(push, 1)

/**
 * @brief Client to Server item move packet
 */
struct TPacketCGItemMove
{
	uint16_t header; ///< The packet header
	uint16_t length; ///< The total length of the packet
	TItemPos pos; ///< The source item position
	TItemPos change_pos; ///< The destination item position
	uint8_t num; ///< The number of items to move
};

#pragma pack(pop)
