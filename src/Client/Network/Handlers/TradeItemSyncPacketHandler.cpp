#include "TradeItemSyncPacketHandler.h"
#include "../Protocol/Protocol.h" // Uzywamy tylko starego glownego Protocol.h gdzie wszystko jest zdefiniowane
// Uzywamy `TPacketGCExchange` z `Protocol.h` aby uniknac podwojnych definicji.

namespace Client::Network::Handlers {

EterBase::PacketResult<void> TradeItemSyncPacketHandler::HandlePacket(std::span<const uint8_t> buffer, EventPublisher publisher)
{
    if (buffer.size() < sizeof(TPacketGCExchange)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCExchange*>(buffer.data());

    if (packet->subheader != ExchangeSub::GC::ITEM_ADD) {
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    // Uzywamy arg2.cell bo Protocol.h (stary pakiet) definiuje arg2 jako TItemPos a komorka to cell
    uint8_t slotIndex = packet->arg2.cell;
    
    // arg1 to Vnum w starym pakiecie
    uint32_t itemVnum = packet->arg1;
    
    // arg3 to count
    uint32_t itemCount = packet->arg3;
    
    bool isMe = packet->is_me != 0;

    TradeItemSyncEvent eventObj(slotIndex, itemVnum, itemCount, isMe);
    
    if (publisher) {
        publisher(eventObj);
    }

    return {};
}

} // namespace Client::Network::Handlers
