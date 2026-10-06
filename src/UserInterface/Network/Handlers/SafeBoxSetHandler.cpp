#include "StdAfx.h"
/**
 * @file SafeBoxSetHandler.cpp
 * @brief Implementation of the SafeBox Item Set network packet handler.
 */
#include "SafeBoxSetHandler.hpp"
#include "../../Packet.h"
#include "../../PythonSafeBox.h"

namespace Network::Handlers {

	bool HandleSafeBoxSet(std::span<const uint8_t> buffer) {
		if (buffer.size() < sizeof(TPacketGCItemSet)) {
			return false;
		}

		const auto* packet = reinterpret_cast<const TPacketGCItemSet*>(buffer.data());

		TItemData itemData{};
		itemData.vnum = packet->vnum;
		itemData.count = packet->count;
		itemData.flags = packet->flags;
		itemData.anti_flags = packet->anti_flags;
		
		for (size_t i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i) {
			itemData.alSockets[i] = packet->alSockets[i];
		}
		
		for (size_t i = 0; i < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++i) {
			itemData.aAttr[i] = packet->aAttr[i];
		}

		CPythonSafeBox::Instance().SetItemData(packet->pos.cell, itemData);

		return true;
	}

} // namespace Network::Handlers
