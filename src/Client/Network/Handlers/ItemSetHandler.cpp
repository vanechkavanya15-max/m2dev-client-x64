#include "ItemSetHandler.h"
#include "../../../EterBase/LogModern.h"
#include "../Protocol/Protocol.h"
#include <algorithm>
#include <format>

namespace Client::Network::Handlers {

EterBase::PacketResult<void> ItemSetHandler::Handle(std::span<const uint8_t> buffer, Gameplay::InventoryDomain& inventoryDomain) {
    if (buffer.size() < sizeof(TPacketGCItemSet)) {
        EterBase::ModernLogger::Error("ItemSetHandler: Buffer too small ({} < {})", buffer.size(), sizeof(TPacketGCItemSet));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCItemSet*>(buffer.data());

    // Zgodnie z wytycznymi, obsluga logiki wymaga tylko SetItem. Sockets/attributes
    // z pakietu sa tu uwzglednione poprzez logowanie, unikajac C4189, az do pelnego
    // wsparcia tych atrybutow w ItemData w nastepnym kroku refaktoryzacji architektonicznej.
    EterBase::ModernLogger::Debug(
        "ItemSetHandler: Deserialized item vnum: {}, count: {}, flags: {}, anti_flags: {}, highlight: {}, first_socket: {}",
        packet->vnum, packet->count, packet->flags, packet->anti_flags, packet->highlight, packet->alSockets[0]
    );

    Gameplay::ItemData itemData{
        .vnum = EterBase::ItemVnum(packet->vnum),
        .count = packet->count,
        .size = {1, 1} // Domyslny rozmiar ze wzgledu na brak w pakiecie
    };

    auto result = inventoryDomain.SetItem(packet->pos.window_type, EterBase::ItemSlot(packet->pos.cell), itemData);

    if (!result.has_value()) {
        EterBase::ModernLogger::Error(
            "ItemSetHandler: SetItem failed for window {}, cell {}: {}",
            packet->pos.window_type, packet->pos.cell, EterBase::ToString(result.error())
        );
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    return {};
}

} // namespace Client::Network::Handlers
