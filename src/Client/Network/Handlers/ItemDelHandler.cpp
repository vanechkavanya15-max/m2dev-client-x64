#include "ItemDelHandler.h"
#include "../../Gameplay/InventoryDomain.h"
#include "../Protocol/Protocol.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/StrongTypes.h"
#include "../Protocol/ProtocolTypes.h"

namespace Client::Network::Handlers {

EterBase::PacketResult<void> ItemDelHandler::HandlePacket(std::span<const uint8_t> buffer, Client::Gameplay::InventoryDomain& inventoryDomain)
{
    if (buffer.size() < sizeof(TPacketGCItemDel)) {
        EterBase::ModernLogger::Error("ItemDelHandler: Buffer size too small: {} < {}", buffer.size(), sizeof(TPacketGCItemDel));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCItemDel*>(buffer.data());

    EterBase::ItemSlot slot(packet->pos.cell);

    auto result = inventoryDomain.RemoveItem(packet->pos.window_type, slot);

    if (!result.has_value()) {
        EterBase::ModernLogger::Error("ItemDelHandler: Failed to remove item at window: {}, cell: {}, error: {}", 
            packet->pos.window_type, packet->pos.cell, EterBase::ToString(result.error()));
        
        // Zwracamy void tak czy siak, logujemy problem ale z punktu widzenia pakietu sieciowego "przeanalizowano" go poprawnie.
        // Jeśli jest to wymagane, można rzucać błąd, ale w sieciowym Handlerze najpewniej lepiej zalogować i przejść dalej bez rozłączania jeśli pakiet ok.
        // Jednak wg opisu: "Zwrócenie odpowiedniego `EterBase::PacketResult`. W razie porażki w usuwaniu z Invetory, zgłoszenie wyjątku/błędu przez `std::unexpected`."
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    return {};
}

} // namespace Client::Network::Handlers
