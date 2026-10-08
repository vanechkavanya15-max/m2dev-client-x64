#pragma once

#include <expected>
#include <cstdint>
#include "Client/Core/Result.h"
#include "Client/Core/DomainCommands.h"
#include "Client/Gameplay/TradeDomain.h"

namespace Client::Gameplay {

class ExchangeStateGuard {
public:
    [[nodiscard]] Client::Core::Result<void, Client::Core::CommandError> Validate(
        const PlayerExchange& exchange,
        EterBase::EntityId initiator,
        EterBase::EntityId target) const;
};

} // namespace Client::Gameplay
