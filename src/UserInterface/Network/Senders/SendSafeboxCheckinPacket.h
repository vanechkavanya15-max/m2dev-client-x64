#pragma once

#include <cstdint>
#include <EterBase/Result.h>
#include <EterBase/StrongTypes.h>

class CNetworkStream;

/**
 * @class SendSafeboxCheckinPacketHandler
 * @brief Handles the creation and transmission of the Safebox Checkin packet to the server.
 * 
 * This class applies SRP (Single Responsibility Principle) by exclusively dealing with network
 * transport for safebox checkin requests (storing an item in the safebox).
 * It adheres to the C++23 standards, avoiding Hungarian notation and strictly decoupling 
 * GUI interaction from network logic.
 */
class SendSafeboxCheckinPacketHandler
{
public:
    /**
     * @brief Sends a safebox checkin request packet to the server.
     * 
     * Applies C++23 guidelines by utilizing `std::expected` (via `EterBase::PacketResult`) 
     * for deterministic error handling and `EterBase::StrongType` to ensure type safety.
     * 
     * @param networkStream Pointer to the network stream used to send the payload.
     * @param safeboxSlot   The index of the safebox slot where the item should be placed.
     * @param inventorySlot The strong type index of the inventory slot from which the item is moved.
     * 
     * @return EterBase::PacketResult<void> Success or an appropriate error code.
     */
    static EterBase::PacketResult<void> Send(CNetworkStream* networkStream, uint8_t safeboxSlot, EterBase::ItemSlot inventorySlot);
};
