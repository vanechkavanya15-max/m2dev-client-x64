#pragma once

#include <vector>
#include <cstdint>
#include <expected>
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"

#include "../../UserInterface/Packet.h"

namespace Client::Network {

/**
 * @brief Command payload for buying an item from a shop.
 */
struct ShopBuyCommand {
    uint8_t count;
    EterBase::ItemSlot position;
};

/**
 * @brief Command payload for selling an item to a shop.
 */
struct ShopSellCommand {
    EterBase::ItemSlot slot;
    uint8_t count;
};

/**
 * @brief Encoder for shop-related CG (Client-to-Game) packets.
 * 
 * Adheres to Zero-Conflict Architecture by separating encoding logic
 * from network transmission state and GUI dependencies.
 */
class ShopCommandEncoder {
public:
    /**
     * @brief Encodes a buy command into a byte buffer.
     * @param cmd The buy command payload.
     * @return PacketResult containing the encoded byte buffer.
     */
    [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeBuy(const ShopBuyCommand& cmd);

    /**
     * @brief Encodes a sell command into a byte buffer.
     * @param cmd The sell command payload.
     * @return PacketResult containing the encoded byte buffer.
     */
    [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeSell(const ShopSellCommand& cmd);
};

} // namespace Client::Network
