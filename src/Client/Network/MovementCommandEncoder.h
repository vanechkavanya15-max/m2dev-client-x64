#pragma once

#include <vector>
#include <cstdint>
#include "../../EterBase/Result.h"
#include "../Core/DomainCommands.h"

namespace Client::Network {

class MovementCommandEncoder {
public:
    [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> Encode(const Core::MoveCommand& cmd);
};

} // namespace Client::Network
