#pragma once

#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include <span>

namespace Client::Gameplay {
    class CombatDomain;
}

namespace Client::Network::Handlers {

class TargetHandler {
public:
    static EterBase::PacketResult<void> HandleGCTarget(std::span<const uint8_t> payload);
};

} // namespace Client::Network::Handlers
