#include "ActorDeleteHandler.h"

namespace Client::Network::Handlers {

    EterBase::PacketResult<void> ActorDeleteHandler::Handle(
        const Client::Network::TPacketGCCharacterDelete& packet,
        Client::World::SpatialHashGrid& spatialGrid,
        Client::Network::IPendingSpawnRegistry& spawnRegistry
    ) {
        auto result = Client::Network::ActorPacketCodec::DecodeCharacterDelete(packet);
        if (!result) {
            return EterBase::MakeError(result.error());
        }

        EterBase::EntityId vid = result.value();

        // Bezpieczne usuwanie ze SpatialHashGrid
        spatialGrid.Remove(vid);

        // Bezpieczne usuwanie z kolejki spawnu (PendingSpawnRegistry)
        // IPendingSpawnRegistry::TakeSpawn returns the item and removes it. 
        // If it lacks additional info, it returns EntityError::InvalidType.
        auto spawnResult = spawnRegistry.TakeSpawn(vid);
        if (!spawnResult) {
            if (spawnResult.error() == EterBase::EntityError::InvalidType) {
                // Workaround: Entity exists but is incomplete. We add dummy info to allow TakeSpawn to remove it.
                Client::Network::TPacketGCCharacterAdditionalInfo dummyInfo{};
                dummyInfo.dwVID = vid.get();
                if (spawnRegistry.RegisterAdditionalInfo(vid, dummyInfo)) {
                    // Ignore result, as the goal is merely to remove it.
                    (void)spawnRegistry.TakeSpawn(vid); 
                }
            }
            // If NotFound, it's already removed or wasn't there, which is fine for a delete operation.
        }

        return {};
    }

} // namespace Client::Network::Handlers
