#pragma once

#include <span>
#include <cstdint>
#include "../../../EterBase/Result.h"

namespace Client::Gameplay {
    class InventoryDomain;
}

namespace Client::Network::Handlers {

/**
 * @class ItemDelHandler
 * @brief Handler odpowiedzialny za parsowanie i procesowanie pakietu GC::ITEM_DEL
 *
 * Zgodnie z zasadą Zero-Conflict z C++23:
 * - Odbiera buffer za pomocą span (bez kopiowania).
 * - Deserializuje TPacketGCItemDel.
 * - Wywołuje InventoryDomain::RemoveItem z pos.window_type oraz pos.cell.
 * - Zwraca EterBase::PacketResult<void>
 */
class ItemDelHandler {
public:
    /**
     * @brief Przetwarza pakiet ITEM_DEL z podanego bufora.
     * @param buffer Bufor bajtów zawierający TPacketGCItemDel.
     * @param inventoryDomain Referencja do domeny ekwipunku, z której usuwany jest element.
     * @return Zwraca EterBase::PacketResult<void> na zakończenie procesu.
     */
    static EterBase::PacketResult<void> HandlePacket(std::span<const uint8_t> buffer, Client::Gameplay::InventoryDomain& inventoryDomain);
};

} // namespace Client::Network::Handlers
