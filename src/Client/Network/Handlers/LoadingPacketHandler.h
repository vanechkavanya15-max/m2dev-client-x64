#pragma once

#include <span>
#include <cstdint>
#include "../../Core/Result.h"
#include "../../Core/WorldContext.h"

namespace Client::Network::Handlers {

class LoadingPacketHandler {
public:
    LoadingPacketHandler() = default;
    ~LoadingPacketHandler() = default;

    Client::Core::Result<void, Client::Core::PacketError> HandleMainCharacter(
        std::span<const uint8_t> payload, 
        Client::Core::WorldContext& context);
};

} // namespace Client::Network::Handlers
