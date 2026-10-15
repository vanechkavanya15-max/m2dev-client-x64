#include "StdAfx.h"
#include "DeadHandler.h"
#include "../CombatPacketCodec.h"

namespace Client::Network::Handlers {

DeadHandler::DeadHandler(MotionStateCallback motionCallback) noexcept
    : m_motionCallback(motionCallback)
{
}

EterBase::PacketResult<void> DeadHandler::ProcessDeadPacket(std::span<const uint8_t> buffer) {
    auto decodeResult = ::Network::CombatPacketCodec::DecodeDead(buffer);
    if (!decodeResult) {
        return EterBase::MakeError(decodeResult.error());
    }

    const auto& packet = decodeResult.value();
    EterBase::EntityId vid{packet.vid};

    if (m_motionCallback) {
        m_motionCallback(vid, ::World::MotionState::Dead);
    }

    return {};
}

} // namespace Client::Network::Handlers
