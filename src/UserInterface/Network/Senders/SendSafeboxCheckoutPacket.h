#pragma once

#include <cstdint>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../GameType.h"

class CNetworkStream;

namespace Network::Senders
{
    /**
     * @brief Handler responsible for sending safebox checkout packets securely to the server.
     * 
     * This class applies SRP (Single Responsibility Principle) by exclusively dealing with network
     * transport for taking items out of the safebox. It separates the game's GUI and visual elements
     * from network logic.
     */
    class SendSafeboxCheckoutHandler
    {
    public:
        /**
         * @brief Sends a safebox checkout request packet to the server.
         * 
         * Applies C++23 guidelines, uses modern Result handling, enforces StrongTypes, 
         * and securely manages memory for the network stream.
         * 
         * @param safeBoxSlot  The slot index within the safebox from which to checkout the item.
         * @param inventoryPos The position in the player's inventory to place the item.
         * @param networkStream Pointer to the network stream used to send the payload.
         * 
         * @return EterBase::PacketResult<void> indicating success or a specific packet error.
         */
        static EterBase::PacketResult<void> SendSafeboxCheckout(EterBase::ItemSlot safeBoxSlot, const TItemPos& inventoryPos, CNetworkStream* networkStream);
    };
}
