#pragma once

#include "../Protocol/Protocol.h"
#include "../../World/SpatialHashGrid.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Client::Network {

class ActorMoveHandler {
public:
    /**
     * @brief Handles the TPacketGCMove network packet.
     * @param packet Pointer to the move packet.
     * @param grid Reference to the spatial hash grid for coordinate updates.
     * @return EterBase::PacketResult<void> indicating success or a specific packet error.
     */
    static EterBase::PacketResult<void> Handle(const TPacketGCMove* packet, Client::World::SpatialHashGrid& grid);
};

} // namespace Client::Network
