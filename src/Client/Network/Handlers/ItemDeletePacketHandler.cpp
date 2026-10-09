#include "ItemDeletePacketHandler.h"
#include "Client/Gameplay/InventoryDomain.h"
#include "Client/Network/Protocol/Protocol.h"
#include "EterBase/LogModern.h"

namespace Client::Network::Handlers {

EterBase::PacketResult<void> ItemDeletePacketHandler::Handle(std::span<const uint8_t> buffer, Client::Gameplay::InventoryDomain& inventoryDomain)
{
    if (buffer.size() < sizeof(TPacketGCItemDel)) {
        EterBase::ModernLogger::Error("ItemDeletePacketHandler: Zbyt maly bufor ({} < {})", buffer.size(), sizeof(TPacketGCItemDel));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCItemDel*>(buffer.data());

    auto result = inventoryDomain.RemoveItem(packet->pos.window_type, EterBase::ItemSlot(packet->pos.cell));

    if (!result.has_value()) {
        EterBase::ModernLogger::Error("ItemDeletePacketHandler: Blad usuniecia przedmiotu (okno: {}, cell: {})", 
                                      packet->pos.window_type, packet->pos.cell);
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    return {};
}

} // namespace Client::Network::Handlers
