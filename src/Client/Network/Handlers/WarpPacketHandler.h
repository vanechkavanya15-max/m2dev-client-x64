#pragma once

#include "EterBase/StdAfx.h"
#include <cstdint>
#include <span>
#include "EterBase/Result.h"
#include "UserInterface/Packet.h"

namespace Client::Network::Handlers {

struct TcpSocketSwitchRequestEvent {
    int32_t x;
    int32_t y;
    uint32_t address;
    uint16_t port;
};

class WarpPacketHandler {
public:
    WarpPacketHandler() = default;
    ~WarpPacketHandler() = default;

    [[nodiscard]] static EterBase::PacketResult<void> HandleWarpPacket(std::span<const uint8_t> payload);
};

} // namespace Client::Network::Handlers
