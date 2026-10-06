/**
 * @file SendExchangeItemPacket.h
 * @brief Modern C++23 declarations for sending exchange (trade) item add packet.
 */

#pragma once

#include "../../StdAfx.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../GameType.h"
#include "../../Core/EventBus.h"
#include <cstdint>
#include <functional>
#include <span>

namespace UserInterface::Core {

    /**
     * @brief Event published when a player offers an item in the exchange window.
     * 
     * The GUI or state manager can subscribe to this event to update the local interface
     * or track pending offers.
     */
    struct ExchangeItemOfferEvent : public IEvent {
        TItemPos itemPos;
        EterBase::ItemSlot displayPos;

        /**
         * @brief Constructs the event.
         * @param item The source position of the item being offered.
         * @param disp The target slot in the exchange UI.
         */
        ExchangeItemOfferEvent(const TItemPos& item, EterBase::ItemSlot disp)
            : itemPos(item), displayPos(disp) {}
    };

} // namespace UserInterface::Core

namespace Network
{
    /**
     * @brief Formats and sends an exchange item add packet to the server.
     * 
     * @param itemPos The inventory or belt position of the item to offer.
     * @param displayPos The target window slot index in the exchange UI.
     * @param sendCallback A callback function taking a span of bytes to send over the network.
     * @return EterBase::PacketResult<void> Returns success or a specific PacketError on failure.
     */
    EterBase::PacketResult<void> SendExchangeItemAddPacket(
        const TItemPos& itemPos, 
        EterBase::ItemSlot displayPos, 
        const std::function<bool(std::span<const uint8_t>)>& sendCallback);
}
