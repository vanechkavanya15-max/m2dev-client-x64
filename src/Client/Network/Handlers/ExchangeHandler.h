#pragma once

#include <span>
#include <cstdint>
#include "EterBase/Result.h"
#include "Client/Gameplay/TradeDomain.h"

namespace Client::Network::Handlers {

class ExchangeHandler {
public:
    static EterBase::PacketResult<void> HandleExchangePacket(
        const std::span<const uint8_t>& payload,
        Client::Gameplay::PlayerExchange* exchangeState,
        EterBase::EntityId selfId,
        EterBase::EntityId targetId);
};

} // namespace Client::Network::Handlers
