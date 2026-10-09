#pragma once

#include <span>
#include <cstdint>
#include <format>
#include <algorithm>

#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "Client/Gameplay/InventoryDomain.h"
#include "Client/Network/Protocol/Packets/Packet_ItemSet.h"
#include "Client/Network/Protocol/ProtocolTypes.h"

namespace Client::Network::Handlers {

/**
 * @brief Handles the item set packet (TPacketGCItemSet) from the server.
 * Odpowiedzialnosc: Parsowanie ID, liczby, socketow i bonusow przedmiotu w ekwipunku.
 */
class ItemSetPacketHandler {
public:
    /**
     * @brief Parses the TPacketGCItemSet buffer and updates the InventoryDomain.
     * 
     * @param buffer The incoming network buffer containing the TPacketGCItemSet.
     * @param inventoryDomain Reference to the inventory domain to update.
     * @return PacketResult indicating success or failure.
     */
    static EterBase::PacketResult<void> Handle(std::span<const uint8_t> buffer, Gameplay::InventoryDomain& inventoryDomain) noexcept {
        if (buffer.size() < sizeof(TPacketGCItemSet)) {
            EterBase::ModernLogger::Error("ItemSetPacketHandler: Buffer too small ({} < {})", buffer.size(), sizeof(TPacketGCItemSet));
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCItemSet*>(buffer.data());

        EterBase::ModernLogger::Debug("ItemSetPacketHandler: Deserialized item vnum: {}, count: {}, flags: {}, anti_flags: {}",
            packet->vnum, packet->count, packet->flags, packet->antiFlags);

        Gameplay::ItemData itemData{
            .vnum = EterBase::ItemVnum(packet->vnum),
            .count = packet->count,
            .size = {1, 1}, // Domyslny rozmiar ze wzgledu na brak w pakiecie
            .sockets = {},
            .attributes = {},
            .flags = packet->flags,
            .anti_flags = packet->antiFlags
        };

        // Kopiowanie socketow
        for (size_t i = 0; i < std::min<size_t>(Client::Network::Protocol::ITEM_SOCKET_SLOT_MAX_NUM, itemData.sockets.size()); ++i) {
            itemData.sockets[i] = packet->sockets[i];
        }

        // Kopiowanie atrybutow
        for (size_t i = 0; i < std::min<size_t>(Client::Network::Protocol::ITEM_ATTRIBUTE_SLOT_MAX_NUM, itemData.attributes.size()); ++i) {
            itemData.attributes[i].type = packet->attributes[i].bType;
            itemData.attributes[i].value = packet->attributes[i].sValue;
        }

        auto result = inventoryDomain.SetItemResult(packet->pos.window_type, Client::Core::SlotIndex(packet->pos.cell), itemData);

        if (!result.has_value()) {
            EterBase::ModernLogger::Error("ItemSetPacketHandler: SetItemResult failed for window {}, cell {}: {}",
                packet->pos.window_type, packet->pos.cell, Client::Core::to_string(result.error()));
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        return {};
    }
};

} // namespace Client::Network::Handlers
