#pragma once

#include <span>
#include <cstdint>
#include <memory>
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "Client/Gameplay/TradeDomain.h"

namespace Client::Network::Handlers {

class TradeStartPacketHandler {
public:
    /**
     * @brief Parsuje pakiet startu wymiany GC i inicjalizuje nowa instancje PlayerExchange.
     * 
     * @param payload Surowy pakiet sieciowy zaczynajacy sie od TPacketGCExchange.
     * @param selfId EntityId lokalnego gracza.
     * @return EterBase::PacketResult zawierajacy unique_ptr do nowo zainicjalizowanego PlayerExchange, 
     *         lub blad w przypadku niepowodzenia.
     */
    static EterBase::PacketResult<std::unique_ptr<Client::Gameplay::PlayerExchange>> HandleTradeStart(
        const std::span<const uint8_t>& payload,
        EterBase::EntityId selfId);
};

} // namespace Client::Network::Handlers
