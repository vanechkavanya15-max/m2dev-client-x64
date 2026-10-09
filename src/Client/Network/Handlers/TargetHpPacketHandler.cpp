#include "TargetHpPacketHandler.h"
#include "../Protocol/Packets/Packet_TargetHP.h"
#include "../../Gameplay/CombatDomain.h"
#include <cstring>

namespace Client::Network::Handlers {

EterBase::PacketResult<void> TargetHpPacketHandler::HandleTargetHpUpdate(std::span<const uint8_t> payload) noexcept {
    if (payload.size() < sizeof(Combat::Packets::TargetHpPacket)) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    Combat::Packets::TargetHpPacket packet;
    std::memcpy(&packet, payload.data(), sizeof(Combat::Packets::TargetHpPacket));

    if (packet.hpPercent > 100) {
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    EterBase::EntityId targetVid(packet.targetId);
    Gameplay::CombatDomain::UpdateTargetHP(targetVid, packet.hpPercent);

    return {};
}

} // namespace Client::Network::Handlers
