#include "CharacterCreateDeleteHandler.h"
#include "../Protocol/Protocol.h"
#include <cstring>

namespace Client::Network::Handlers {

EterBase::PacketResult<void> CharacterCreateDeleteHandler::HandleCreateFailure(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCCreateFailure)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCCreateFailure packet;
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCCreateFailure));

    Client::Core::EventBus::Instance().Publish(CharacterCreateFailureEvent(packet.bType));
    
    return {};
}

EterBase::PacketResult<void> CharacterCreateDeleteHandler::HandleDeleteFailure(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCBlank)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    Client::Core::EventBus::Instance().Publish(CharacterDeleteFailureEvent());

    return {};
}

} // namespace Client::Network::Handlers
