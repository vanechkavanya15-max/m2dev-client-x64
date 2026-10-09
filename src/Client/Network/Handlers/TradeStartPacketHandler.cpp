#include "TradeStartPacketHandler.h"
#include "Client/Network/Protocol/Packets/Packet_Exchange.h"
#include "Client/Network/Protocol/Protocol.h"

namespace Client::Network::Handlers {

EterBase::PacketResult<std::unique_ptr<Client::Gameplay::PlayerExchange>> TradeStartPacketHandler::HandleTradeStart(
    const std::span<const uint8_t>& payload,
    EterBase::EntityId selfId)
{
    if (payload.size() < sizeof(TPacketGCExchange)) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCExchange*>(payload.data());
    
    if (packet->subheader != ExchangeSub::GC::START) {
        return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
    }

    EterBase::EntityId targetId{packet->value1};

    if (selfId == targetId) {
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload); // Zakaz wymiany z samym soba
    }

    // Ustalanie rol. Flaga isInitiator okresla, czy klient jest inicjatorem wymiany.
    // Jesli isInitiator jest ustawione, klient (selfId) zainicjowal wymiane, a celem jest targetId.
    // W przeciwnym razie targetId zainicjowal wymiane, a celem jest klient (selfId).
    EterBase::EntityId initiatorId = packet->isInitiator ? selfId : targetId;
    EterBase::EntityId otherId = packet->isInitiator ? targetId : selfId;

    auto exchange = std::make_unique<Client::Gameplay::PlayerExchange>(initiatorId, otherId);
    
    return exchange;
}

} // namespace Client::Network::Handlers
