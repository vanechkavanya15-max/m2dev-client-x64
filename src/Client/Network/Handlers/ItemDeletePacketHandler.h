#pragma once

#include <span>
#include <cstdint>
#include "EterBase/Result.h"

namespace Client::Gameplay {
    class InventoryDomain;
}

namespace Client::Network::Handlers {

/**
 * @class ItemDeletePacketHandler
 * @brief Bezpieczny handler pakietu GC::ITEM_DEL realizujacy usuniecie przedmiotu.
 *
 * Architektura AI-First:
 * - Czysta funkcja bezstanowa.
 * - Bezpieczne rzutowanie ze std::span.
 * - Brak raw pointers.
 */
class ItemDeletePacketHandler {
public:
    static EterBase::PacketResult<void> Handle(std::span<const uint8_t> buffer, Client::Gameplay::InventoryDomain& inventoryDomain);
};

} // namespace Client::Network::Handlers
