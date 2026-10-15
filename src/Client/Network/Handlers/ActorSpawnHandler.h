#pragma once

#include <span>
#include <cstdint>
#include <EterBase/Result.h>
#include <EterBase/StrongTypes.h>

namespace Client::Network {
class IPendingSpawnRegistry;
}

namespace Client::Network::Handlers {

class ActorSpawnHandler {
public:
    ActorSpawnHandler() = default;
    ~ActorSpawnHandler() = default;

    EterBase::PacketResult<void> HandleActorSpawn(
        std::span<const uint8_t> payload, 
        Client::Network::IPendingSpawnRegistry& registry);
};

} // namespace Client::Network::Handlers
