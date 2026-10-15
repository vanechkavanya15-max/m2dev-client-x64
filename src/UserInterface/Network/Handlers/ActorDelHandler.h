#pragma once

#include <cstdint>
#include <span>

class CNetworkActorManager;

namespace Network::Handlers
{
    /**
     * @brief Handles character deletion packet payload for the actor manager.
     * 
     * @param payload Binary span representing the incoming network packet.
     * @param actorManager Reference to the central actor manager for actor removal.
     * @return true if the packet was successfully parsed and the actor was removed; otherwise false.
     */
    bool HandleActorDel(std::span<const uint8_t> payload, CNetworkActorManager& actorManager);
}
