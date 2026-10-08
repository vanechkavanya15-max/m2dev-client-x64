#include "DeadHandler.h"
#include "../CombatPacketCodec.h"

namespace Client::Network::Handlers {

DeadHandler::DeadHandler(MotionStateCallback motionCallback)
    : m_motionCallback(std::move(motionCallback)) {}

EterBase::PacketResult<void> DeadHandler::ProcessDeadPacket(std::span<const uint8_t> buffer) {
    auto decodeResult = CombatPacketCodec::DecodeDead(buffer);
    if (!decodeResult) {
        return EterBase::MakeError(decodeResult.error());
    }

    const auto& packet = decodeResult.value();
    Client::World::EntityVid vid{packet.vid};

    if (m_motionCallback) {
        m_motionCallback(vid, World::MotionState::Dead);
    }

    return {};
}

} // namespace Client::Network::Handlers
