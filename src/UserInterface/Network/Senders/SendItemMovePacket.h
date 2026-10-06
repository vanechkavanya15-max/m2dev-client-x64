#pragma once

#include "../../../EterBase/Result.h"
#include "../../GameType.h"
#include <cstdint>
#include <functional>
#include <span>

namespace Network
{
    /**
     * @brief Formats and sends an item move packet to the server.
     * 
     * @param sourcePos The original position of the item in the inventory/safebox.
     * @param targetPos The destination position for the item.
     * @param count The number of items to move.
     * @param sendCallback A callback function taking a span of bytes to send over the network.
     * @return EterBase::PacketResult<void> Returns expected success or an error if sending fails.
     */
    EterBase::PacketResult<void> SendItemMovePacket(const TItemPos& sourcePos, 
                                                    const TItemPos& targetPos, 
                                                    uint8_t count, 
                                                    const std::function<bool(std::span<const uint8_t>)>& sendCallback);
}
