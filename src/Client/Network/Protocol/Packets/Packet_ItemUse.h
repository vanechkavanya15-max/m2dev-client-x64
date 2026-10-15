#pragma once

#include <cstdint>
#include "../GameType.h"

#pragma pack(push, 1)

/**
 * @brief Network packet structure for item usage requests from client to server (HEADER_CG_ITEM_USE = 11)
 * @details Sent when a user attempts to consume, equip or otherwise activate an item from their inventory.
 * Contains only the slot position; all logic is executed on the server-side.
 */
struct TPacketCGItemUse
{
	/** @brief Network command header (HEADER_CG_ITEM_USE) */
	uint8_t header;

	/** @brief Location of the item to be used (Window type and cell index) */
	TItemPos pos;

	/**
	 * @brief Validates if the position is within valid inventory bounds.
	 * @return true if valid, false otherwise.
	 */
	bool IsValid() const
	{
		// Note: TItemPos has its own IsValidCell() which checks window limits (e.g. c_Inventory_Count)
		// Since TItemPos is modified in const methods, we const_cast to call IsValidCell
		return const_cast<TItemPos*>(&pos)->IsValidCell();
	}
};

/**
 * @brief Network packet structure for using an item on another item (HEADER_CG_ITEM_USE_TO_ITEM = 60)
 * @details Sent when a user drag & drops one item onto another (e.g., adding gems to weapons or using upgrade materials).
 */
struct TPacketCGItemUseToItem
{
	/** @brief Network command header (HEADER_CG_ITEM_USE_TO_ITEM) */
	uint8_t header;

	/** @brief Location of the item being used (source item) */
	TItemPos sourcePos;

	/** @brief Location of the item being targeted (target item) */
	TItemPos targetPos;

	/**
	 * @brief Validates if the positions are within valid bounds.
	 * @return true if valid, false otherwise.
	 */
	bool IsValid() const
	{
		return const_cast<TItemPos*>(&sourcePos)->IsValidCell() &&
			const_cast<TItemPos*>(&targetPos)->IsValidCell();
	}
};

#pragma pack(pop)
