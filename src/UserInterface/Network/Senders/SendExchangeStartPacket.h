/**
 * @file SendExchangeStartPacket.h
 * @brief Modern C++23 network sender for initiating a trade/exchange with another player.
 */

#pragma once

#include <cstdint>
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/Result.h"

class CNetworkStream;

/**
 * @class SendExchangeStartPacketHandler
 * @brief Handler responsible for sending the exchange start request packet to the server.
 * 
 * This class applies the Single Responsibility Principle by exclusively dealing with network
 * transport for exchange initialization. It separates the game's GUI state from the network logic.
 */
class SendExchangeStartPacketHandler
{
public:
    /**
     * @brief Sends an exchange start request packet to the server for a specific target.
     * 
     * Applies C++23 guidelines, uses strong types to prevent ID mismatches, and returns
     * deterministically using std::expected (EterBase::PacketResult).
     * 
     * @param targetId      The EntityId (Virtual ID) of the target player to trade with.
     * @param networkStream Pointer to the network stream used to send the payload.
     * 
     * @return PacketResult<void> Returns success or a specific PacketError if transmission fails.
     */
    static EterBase::PacketResult<void> SendExchangeStart(EterBase::EntityId targetId, CNetworkStream* networkStream);
};
