#include "../../../EterBase/StdAfx.h"
#pragma once

#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../ActorPacketCodec.h"
#include "../../World/SpatialHashGrid.h"
#include "../PendingSpawnRegistry.h"

namespace Client::Network::Handlers {

    class ActorDeleteHandler {
    public:
        static EterBase::PacketResult<void> Handle(
            const Client::Network::TPacketGCCharacterDelete& packet,
            Client::World::SpatialHashGrid& spatialGrid,
            Client::Network::IPendingSpawnRegistry& spawnRegistry
        );
    };

} // namespace Client::Network::Handlers
