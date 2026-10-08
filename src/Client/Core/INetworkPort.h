#pragma once

#include <cstdint>
#include <span>
#include "Result.h"

namespace Client::Core {

class INetworkPort {
public:
    virtual ~INetworkPort() = default;

    [[nodiscard]] virtual Result<void, PacketError> SendRaw(uint8_t opcode, std::span<const uint8_t> payload) = 0;
    [[nodiscard]] virtual bool IsConnected() const noexcept = 0;
};

} // namespace Client::Core
