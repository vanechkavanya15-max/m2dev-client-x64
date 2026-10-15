#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <expected>
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"

class CNetworkActorManager;

namespace Network::Handlers
{
    /**
     * @brief Parses and handles the HEADER_GC_CHAR_ADDITIONAL_INFO packet.
     * 
     * Extracts additional information about a character (e.g., titles, effects, 
     * specific parts, level, and alignment) from the network packet and updates 
     * the internal C++ state via the CNetworkActorManager. It also emits an event
     * to refresh the UI decoupled from the logic.
     * 
     * @param payload A binary span containing the TPacketGCCharacterAdditionalInfo packet.
     * @param actorManager Reference to the actor manager which stores the character data.
     * @return EterBase::PacketResult<void> Success or PacketError.
     */
    EterBase::PacketResult<void> HandleCharAdditionalInfo(std::span<const uint8_t> payload, CNetworkActorManager& actorManager);
}
