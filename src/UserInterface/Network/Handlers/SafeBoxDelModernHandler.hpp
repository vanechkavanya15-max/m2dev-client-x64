/**
 * @file SafeBoxDelModernHandler.hpp
 * @brief Handler for the SafeBox Item Del network packet in C++23.
 */
#pragma once

#include <span>
#include <cstdint>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

namespace Network::Handlers {

	/**
	 * @brief Event emitted when an item is deleted from the SafeBox.
	 */
	struct SafeBoxItemDeleteEvent : public UserInterface::Core::IEvent {
		EterBase::ItemSlot slot;

		/**
		 * @brief Constructs a new SafeBoxItemDeleteEvent.
		 * @param slot The slot index from which the item was deleted.
		 */
		explicit SafeBoxItemDeleteEvent(EterBase::ItemSlot slot) : slot(slot) {}
	};

	/**
	 * @brief Handles the incoming packet for deleting an item from the safebox.
	 * 
	 * @param buffer The binary buffer containing the TPacketGCItemDel packet.
	 * @return PacketResult<void> Returns success or a specific packet error.
	 */
	EterBase::PacketResult<void> HandleSafeBoxDel(std::span<const uint8_t> buffer);

} // namespace Network::Handlers
