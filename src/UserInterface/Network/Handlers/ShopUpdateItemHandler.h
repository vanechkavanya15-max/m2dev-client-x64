#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

namespace Network::Handlers
{
    /**
     * @brief Event emitted when a shop item is updated.
     * 
     * Subsystems like the UI can subscribe to this event to refresh
     * the shop view without being tightly coupled to the network handler.
     */
    struct ShopItemUpdatedEvent : public UserInterface::Core::IEvent
    {
        EterBase::ItemSlot position;

        /**
         * @brief Constructs the shop item updated event.
         * @param pos The slot position of the updated item.
         */
        explicit ShopItemUpdatedEvent(EterBase::ItemSlot pos) : position(pos) {}
    };

    /**
     * @brief Processes the TPacketGCShopUpdateItem packet.
     * 
     * This function parses the incoming binary buffer, updates the local CPythonShop
     * instance memory state, and triggers a ShopItemUpdatedEvent.
     * 
     * @param buffer The binary data span containing the packet.
     * @return EterBase::PacketResult<void> Returns a strict success or error status.
     */
    EterBase::PacketResult<void> ProcessShopUpdateItem(std::span<const uint8_t> buffer);

    /**
     * @brief Compatible handler interface for packet dispatching.
     * @param buffer The binary data span containing the packet.
     * @return true if the packet was successfully processed.
     */
    bool HandleShopUpdateItem(std::span<const uint8_t> buffer);
}
