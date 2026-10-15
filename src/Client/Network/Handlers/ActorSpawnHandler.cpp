#include "ActorSpawnHandler.h"
#include "../Protocol/Protocol.h"
#include "../PendingSpawnRegistry.h"
#include <cstring>

namespace Client::Network::Handlers {

EterBase::PacketResult<void> ActorSpawnHandler::HandleActorSpawn(
    std::span<const uint8_t> payload, 
    Client::Network::IPendingSpawnRegistry& registry) 
{
    if (payload.size() < sizeof(TPacketGCCharacterAdd)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCCharacterAdd packet;
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCCharacterAdd));

    EterBase::EntityId vid(packet.dwVID);

    auto result = registry.RegisterSpawn(vid, packet);
    if (!result.has_value()) {
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    return {};
}

} // namespace Client::Network::Handlers
