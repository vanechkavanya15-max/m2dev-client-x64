#pragma once

#include <span>
#include <cstdint>
#include "EterBase/Result.h"

namespace Client::Gameplay {
    class NpcShop;
}

namespace Client::Network::Handlers {

/**
 * @brief Zero-Conflict C++23 handler for Shop packets.
 * 
 * Deserializes TPacketGCShop and its subtypes (START, UPDATE_ITEM, END)
 * and populates the domain logic within NpcShop securely without using 
 * naked pointers or bypassing the bounds checking.
 */
class ShopHandler {
public:
    /**
     * @brief Parses the Shop packet and delegates state changes to the TradeDomain.
     * 
     * @param payload The binary data representing the packet starting from TPacketGCShop.
     * @param shop_domain The domain service to populate with decoded shop elements.
     * @return EterBase::PacketResult<void> Success or PacketError (e.g. BufferUnderflow).
     */
    static EterBase::PacketResult<void> HandleShopPacket(
        std::span<const uint8_t> payload, 
        Client::Gameplay::NpcShop& shop_domain);
};

} // namespace Client::Network::Handlers
