#include "EterBase/StdAfx.h"
#include "WarpPacketHandler.h"
#include "UserInterface/Core/EventBus.h"
#include <cstring>

namespace Client::Network::Handlers {

EterBase::PacketResult<void> WarpPacketHandler::HandleWarpPacket(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCWarp)) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCWarp packet{};
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCWarp));

    if (packet.wPort == 0) {
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    TcpSocketSwitchRequestEvent ev{
        .x = packet.lX,
        .y = packet.lY,
        .address = static_cast<uint32_t>(packet.lAddr),
        .port = packet.wPort
    };

    UserInterface::Core::EventBus::GetInstance().Publish(ev);
    return {};
}

} // namespace Client::Network::Handlers
