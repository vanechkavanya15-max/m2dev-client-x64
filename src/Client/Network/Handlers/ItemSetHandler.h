#pragma once

#include <span>
#include <cstdint>
#include "../../../EterBase/Result.h"
#include "../../Gameplay/InventoryDomain.h"

namespace Client::Network::Handlers {

/**
 * @brief Handles the item set packet (TPacketGCItemSet) from the server.
 */
class ItemSetHandler {
public:
    /**
     * @brief Parses the TPacketGCItemSet buffer and updates the InventoryDomain.
     * 
     * @param buffer The incoming network buffer containing the TPacketGCItemSet.
     * @param inventoryDomain Reference to the inventory domain to update.
     * @return PacketResult indicating success or failure.
     */
    static EterBase::PacketResult<void> Handle(std::span<const uint8_t> buffer, Gameplay::InventoryDomain& inventoryDomain);
};

} // namespace Client::Network::Handlers
