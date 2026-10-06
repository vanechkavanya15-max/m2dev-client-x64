/**
 * @file SendShopPacket.h
 * @brief Network packet senders for Shop interactions.
 * 
 * Contains modern C++23 implementations for sending shop-related packets 
 * from the client to the server, decoupled from GUI logic.
 */
#pragma once

#include <cstdint>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Network::Senders
{
    /**
     * @brief Sends a packet to end shop interaction.
     * @return EterBase::PacketResult<void> indicating success or a PacketError.
     */
    EterBase::PacketResult<void> SendShopEnd();

    /**
     * @brief Sends a packet to buy an item from the shop.
     * @param count The quantity of the item to buy.
     * @param position The grid position of the item in the shop.
     * @return EterBase::PacketResult<void> indicating success or a PacketError.
     */
    EterBase::PacketResult<void> SendShopBuy(uint8_t count, EterBase::ItemSlot position);

    /**
     * @brief Sends a legacy packet to sell an item to the shop.
     * @param slot The inventory slot of the item to sell.
     * @return EterBase::PacketResult<void> indicating success or a PacketError.
     */
    EterBase::PacketResult<void> SendShopSell(EterBase::ItemSlot slot);

    /**
     * @brief Sends a modern packet to sell an item to the shop with quantity.
     * @param slot The inventory slot of the item to sell.
     * @param count The quantity of the item to sell.
     * @return EterBase::PacketResult<void> indicating success or a PacketError.
     */
    EterBase::PacketResult<void> SendShopSellNew(EterBase::ItemSlot slot, uint8_t count);
}
