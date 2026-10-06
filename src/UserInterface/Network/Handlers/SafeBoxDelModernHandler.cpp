#include "StdAfx.h"
/**
 * @file SafeBoxDelModernHandler.cpp
 * @brief Implementation of the C++23 SafeBox Item Del network packet handler.
 */
#include "SafeBoxDelModernHandler.hpp"
#include "../../Packet.h"
#include "../../PythonSafeBox.h"
#include "../../../EterBase/LogModern.h"

namespace Network::Handlers {

	EterBase::PacketResult<void> HandleSafeBoxDel(std::span<const uint8_t> buffer) {
		if (buffer.size() < sizeof(TPacketGCItemDel)) {
			EterBase::ModernLogger::Error("HandleSafeBoxDel: Buffer underflow (expected {}, got {})", 
				sizeof(TPacketGCItemDel), buffer.size());
			return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
		}

		const auto* packet = reinterpret_cast<const TPacketGCItemDel*>(buffer.data());

		// Extract the safe box cell position
		EterBase::ItemSlot slotId{packet->pos.cell};

		// Safely update the C++ memory state by deleting the item
		CPythonSafeBox::Instance().DelItemData(slotId.value());

		// Log the action safely using C++23 std::format wrapped in ModernLogger
		EterBase::ModernLogger::Debug("HandleSafeBoxDel: Removed item from safebox slot {}", slotId);

		// Emit an event to decouple the network layer from the Python UI
		UserInterface::Core::EventBus::GetInstance().Publish(SafeBoxItemDeleteEvent{slotId});

		return {};
	}

} // namespace Network::Handlers
