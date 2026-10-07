#pragma once

#include <span>
#include <cstdint>
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "Client/Network/ActorPacketCodec.h"
#include "Client/Network/PendingSpawnRegistry.h"

namespace Client::Network::Handlers {

    /**
     * @brief Parses and handles the HEADER_GC_CHAR_ADDITIONAL_INFO packet.
     * 
     * Extracts additional information about a character (e.g., name, guild, PvP flag, level)
     * from the network packet and registers it with the PendingSpawnRegistry to complete
     * the spawn process. Adheres to the Zero-Conflict rule.
     * 
     * @param payload A binary span containing the TPacketGCCharacterAdditionalInfo packet.
     * @param registry Reference to the IPendingSpawnRegistry.
     * @return EterBase::PacketResult<void> Success or PacketError.
     */
    EterBase::PacketResult<void> HandleActorAdditionalInfo(std::span<const uint8_t> payload, Client::Network::IPendingSpawnRegistry& registry);

} // namespace Client::Network::Handlers
