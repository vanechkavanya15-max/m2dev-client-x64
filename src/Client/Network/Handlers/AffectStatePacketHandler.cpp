#include "AffectStatePacketHandler.h"
#include "../Protocol/Protocol.h"
#include <cstring>

namespace Client::Network::Handlers {

EterBase::PacketResult<void> AffectStatePacketHandler::HandleAffectAdd(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCAffectAdd)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCAffectAdd packet;
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCAffectAdd));

    Client::Core::Events::AffectAddEvent event(
        packet.elem.dwType,
        packet.elem.bPointIdxApplyOn,
        packet.elem.lApplyValue,
        packet.elem.dwFlag,
        packet.elem.lDuration,
        packet.elem.lSPCost
    );

    Client::Core::EventBus::GetInstance().Publish(event);

    return {};
}

EterBase::PacketResult<void> AffectStatePacketHandler::HandleAffectRemove(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCAffectRemove)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCAffectRemove packet;
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCAffectRemove));

    Client::Core::Events::AffectRemoveEvent event(
        packet.dwType,
        packet.bApplyOn
    );

    Client::Core::EventBus::GetInstance().Publish(event);

    return {};
}

} // namespace Client::Network::Handlers
