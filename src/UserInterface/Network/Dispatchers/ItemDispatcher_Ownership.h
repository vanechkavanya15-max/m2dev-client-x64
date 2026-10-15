#pragma once

#include <span>
#include <cstdint>
#include "../../../EterBase/Result.h"

namespace UserInterface::Network {

/**
 * @brief Dispatches the item ownership packet (TPacketGCItemOwnership).
 * 
 * Extracts the item's Entity ID and the new owner's name, then publishes an event
 * via the EventBus for the UI and core game loop to react to.
 * 
 * @param buffer Raw byte span representing the packet payload.
 * @return PacketResult<void> Returns success or a PacketError on failure.
 */
EterBase::PacketResult<void> DispatchItemOwnership(std::span<const uint8_t> buffer);

} // namespace UserInterface::Network
