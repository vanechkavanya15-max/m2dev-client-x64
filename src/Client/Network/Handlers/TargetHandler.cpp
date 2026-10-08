#include "EterBase/StdAfx.h"
#include "../Protocol/Protocol.h"
#include "TargetHandler.h"
#include "../../Gameplay/CombatDomain.h"

namespace Client::Network::Handlers {

EterBase::PacketResult<void> TargetHandler::HandleGCTarget(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCTarget)) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    // Safely copy to avoid strict aliasing / alignment issues
    TPacketGCTarget packet;
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCTarget));

    EterBase::EntityId targetVid(packet.dwVID);
    uint8_t hpPercent = packet.bHPPercent;

    if (hpPercent > 100) {
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    Gameplay::CombatDomain::UpdateTargetHP(targetVid, hpPercent);
    
    return {};
}

} // namespace Client::Network::Handlers
