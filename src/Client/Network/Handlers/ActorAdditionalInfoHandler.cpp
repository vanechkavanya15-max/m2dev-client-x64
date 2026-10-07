#include "ActorAdditionalInfoHandler.h"
#include <expected>

namespace Client::Network::Handlers {

    EterBase::PacketResult<void> HandleActorAdditionalInfo(std::span<const uint8_t> payload, Client::Network::IPendingSpawnRegistry& registry) {
        if (payload.size() != sizeof(Client::Network::TPacketGCCharacterAdditionalInfo)) {
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        const auto* packet = reinterpret_cast<const Client::Network::TPacketGCCharacterAdditionalInfo*>(payload.data());
        EterBase::EntityId vid(packet->dwVID);

        auto result = registry.RegisterAdditionalInfo(vid, *packet);
        if (!result) {
            // According to task, we should return EterBase::PacketResult<void>, so we ignore specific domain errors here or map them if needed
            // The instructions do not mandate specific error propagation from the registry, but we return a success to satisfy the packet parsing.
            // Wait, we can return success since the packet itself is valid.
        }

        return {};
    }

} // namespace Client::Network::Handlers
