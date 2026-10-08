#include "../../EterBase/StdAfx.h"
#include "ActorMoveHandler.h"

#include "../../EterBase/ModernLogger.h"

namespace Client::Network {

// Wartosci funkcji ruchu (CInstanceBase::FUNC_WAIT / FUNC_MOVE)
constexpr uint8_t FUNC_WAIT = 0;
constexpr uint8_t FUNC_MOVE = 1;

EterBase::PacketResult<void> ActorMoveHandler::Handle(const TPacketGCMove* packet, Client::World::SpatialHashGrid& grid) {
    if (!packet) {
        EterBase::ModernLogger::Error("ActorMoveHandler: packet is null");
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    // Walidacja bFunc (Walk, Run)
    if (packet->bFunc != FUNC_MOVE && packet->bFunc != FUNC_WAIT) {
        EterBase::ModernLogger::Error("ActorMoveHandler: invalid bFunc value {}", packet->bFunc);
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    EterBase::EntityId vid{packet->dwVID};
    float x = static_cast<float>(packet->lX);
    float y = static_cast<float>(packet->lY);

    grid.Update(vid, x, y);

    EterBase::ModernLogger::Debug("ActorMoveHandler: Updated entity {} at ({}, {}) func: {}", 
                                   packet->dwVID, x, y, packet->bFunc);

    return {};
}

} // namespace Client::Network
