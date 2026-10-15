#pragma once

#include <span>
#include <cstdint>
#include "../../../EterBase/Result.h"
#include "../../Gameplay/TradeDomain.h"

namespace Client::Network::Handlers {

class TradeConfirmPacketHandler {
public:
    /**
     * @brief Przetwarza pakiet zatwierdzenia wymiany i aplikuje logike do sesji gracza.
     * 
     * @param payload Skompresowane dane sieciowe zawierajace TPacketGCExchange.
     * @param exchangeState Wskaznik do obecnego stanu wymiany miedzy graczami.
     * @param selfId Identyfikator wlasny (gracza odbierajacego).
     * @param targetId Identyfikator celu wymiany.
     * @return EterBase::PacketResult<void> ze statusem operacji.
     */
    static EterBase::PacketResult<void> Handle(
        const std::span<const uint8_t>& payload,
        Client::Gameplay::PlayerExchange* exchangeState,
        EterBase::EntityId selfId,
        EterBase::EntityId targetId);
};

} // namespace Client::Network::Handlers
