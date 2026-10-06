#pragma once

#include <cstdint>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../GameType.h"

class CNetworkStream;

namespace UserInterface::Network::Senders
{
    /**
     * @brief Handler responsible for sending item drop packets securely to the server.
     * 
     * Applies C++23 guidelines and avoids Hungarian notation. Separates network sending logic
     * from the UI layer to maintain the Single Responsibility Principle.
     */
    class SendItemDropPacket
    {
    public:
        /**
         * @brief Sends an item drop request to the server (legacy format without count, using elk/gold).
         * 
         * @param windowType    The window type (e.g., INVENTORY) where the item is located.
         * @param itemSlot      The specific slot of the item to drop.
         * @param elk           The amount of elk (gold) associated with the drop.
         * @param networkStream Pointer to the network stream used to send the payload.
         * 
         * @return EterBase::PacketResult<void> representing success or strict failure state.
         */
        static EterBase::PacketResult<void> SendItemDrop(uint8_t windowType, EterBase::ItemSlot itemSlot, uint32_t elk, CNetworkStream* networkStream);

        /**
         * @brief Sends an item drop request to the server (new format with count).
         * 
         * @param windowType    The window type (e.g., INVENTORY) where the item is located.
         * @param itemSlot      The specific slot of the item to drop.
         * @param gold          The amount of gold associated with the drop.
         * @param count         The specific count of the item to drop.
         * @param networkStream Pointer to the network stream used to send the payload.
         * 
         * @return EterBase::PacketResult<void> representing success or strict failure state.
         */
        static EterBase::PacketResult<void> SendItemDropNew(uint8_t windowType, EterBase::ItemSlot itemSlot, uint32_t gold, uint8_t count, CNetworkStream* networkStream);
    };
}
