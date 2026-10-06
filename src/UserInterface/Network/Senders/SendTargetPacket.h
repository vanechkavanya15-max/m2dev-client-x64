#pragma once

#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

class CNetworkStream;

namespace UserInterface::Network::Senders {

/**
 * @class SendTargetPacket
 * @brief Modern C++23 Network Sender for the CG::TARGET packet.
 *
 * Implements the outbound logic for selecting/highlighting a target entity in the game world.
 * This strict implementation prevents bypassing to Python UI modules directly, returning deterministic
 * states using EterBase::PacketResult.
 */
class SendTargetPacket {
public:
    /**
     * @brief Sends a target packet to the server to highlight an entity.
     * 
     * @param stream Reference to the initialized network stream used for transmission.
     * @param targetId The securely-typed EntityId representing the target to select.
     * @return EterBase::PacketResult<void> Returns success or a specific packet error on failure.
     */
    static EterBase::PacketResult<void> Execute(CNetworkStream& stream, EterBase::EntityId targetId);
};

} // namespace UserInterface::Network::Senders
