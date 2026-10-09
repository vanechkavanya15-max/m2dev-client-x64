#include "ActorAdditionalInfoPacketHandler.h"
#include "Client/Network/Protocol/Protocol.h"
#include <expected>

namespace Client::Network::Handlers {

    EterBase::PacketResult<void> ActorAdditionalInfoPacketHandler::Handle(std::span<const uint8_t> payload, Client::Network::IPendingSpawnRegistry& registry) {
        if (payload.size() != sizeof(::TPacketGCCharacterAdditionalInfo)) {
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        const auto* packet = reinterpret_cast<const ::TPacketGCCharacterAdditionalInfo*>(payload.data());
        EterBase::EntityId vid(packet->dwVID);

        auto result = registry.RegisterAdditionalInfo(vid, *packet);
        if (!result) {
            if (result.error() == EterBase::EntityError::NotFound) {
                return std::unexpected(EterBase::PacketError::SequenceMismatch);
            }
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        return {};
    }

} // namespace Client::Network::Handlers
