#include "StdAfx.h"
/**
 * @file SendShopPacket.cpp
 * @brief Implementation of network packet senders for Shop interactions.
 * 
 * Contains modern C++23 implementations decoupled from GUI logic.
 * Enforces Zero-Conflict Rule by being in a new dedicated file.
 */

#include "SendShopPacket.h"
#include "../../PythonNetworkStream.h"
#include "../../Packet.h"
#include "../../../EterBase/LogModern.h"

namespace Network::Senders
{
    /**
     * @brief Sends a packet to end shop interaction.
     * @return EterBase::PacketResult<void> indicating success or a SessionClosed error.
     */
    EterBase::PacketResult<void> SendShopEnd()
    {
        auto& stream = CPythonNetworkStream::Instance();

        TPacketCGShop packetShop;
        packetShop.header = CG::SHOP;
        packetShop.length = sizeof(packetShop);
        packetShop.subheader = ShopSub::CG::END;

        if (!stream.Send(sizeof(packetShop), &packetShop))
        {
            EterBase::ModernLogger::Error("SendShopEnd: Failed to send TPacketCGShop.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        return {};
    }

    /**
     * @brief Sends a packet to buy an item from the shop.
     * @param count The quantity of the item to buy.
     * @param position The grid position of the item in the shop.
     * @return EterBase::PacketResult<void> indicating success or a SessionClosed error.
     */
    EterBase::PacketResult<void> SendShopBuy(uint8_t count, EterBase::ItemSlot position)
    {
        auto& stream = CPythonNetworkStream::Instance();

        TPacketCGShop packetShop;
        packetShop.header = CG::SHOP;
        packetShop.length = sizeof(packetShop) + sizeof(uint8_t) + sizeof(uint8_t);
        packetShop.subheader = ShopSub::CG::BUY;

        if (!stream.Send(sizeof(packetShop), &packetShop))
        {
            EterBase::ModernLogger::Error("SendShopBuy: Failed to send TPacketCGShop header.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        if (!stream.Send(sizeof(uint8_t), &count))
        {
            EterBase::ModernLogger::Error("SendShopBuy: Failed to send count.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        uint8_t rawPosition = static_cast<uint8_t>(position.value());
        if (!stream.Send(sizeof(uint8_t), &rawPosition))
        {
            EterBase::ModernLogger::Error("SendShopBuy: Failed to send position.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        return {};
    }

    /**
     * @brief Sends a legacy packet to sell an item to the shop.
     * @param slot The inventory slot of the item to sell.
     * @return EterBase::PacketResult<void> indicating success or a SessionClosed error.
     */
    EterBase::PacketResult<void> SendShopSell(EterBase::ItemSlot slot)
    {
        auto& stream = CPythonNetworkStream::Instance();

        TPacketCGShop packetShop;
        packetShop.header = CG::SHOP;
        packetShop.length = sizeof(packetShop) + sizeof(uint8_t);
        packetShop.subheader = ShopSub::CG::SELL;

        if (!stream.Send(sizeof(packetShop), &packetShop))
        {
            EterBase::ModernLogger::Error("SendShopSell: Failed to send TPacketCGShop header.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        uint8_t rawSlot = static_cast<uint8_t>(slot.value());
        if (!stream.Send(sizeof(uint8_t), &rawSlot))
        {
            EterBase::ModernLogger::Error("SendShopSell: Failed to send slot.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        return {};
    }

    /**
     * @brief Sends a modern packet to sell an item to the shop with quantity.
     * @param slot The inventory slot of the item to sell.
     * @param count The quantity of the item to sell.
     * @return EterBase::PacketResult<void> indicating success or a SessionClosed error.
     */
    EterBase::PacketResult<void> SendShopSellNew(EterBase::ItemSlot slot, uint8_t count)
    {
        auto& stream = CPythonNetworkStream::Instance();

        TPacketCGShop packetShop;
        packetShop.header = CG::SHOP;
        packetShop.length = sizeof(packetShop) + sizeof(uint8_t) + sizeof(uint8_t);
        packetShop.subheader = ShopSub::CG::SELL2;

        if (!stream.Send(sizeof(packetShop), &packetShop))
        {
            EterBase::ModernLogger::Error("SendShopSellNew: Failed to send TPacketCGShop header.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        uint8_t rawSlot = static_cast<uint8_t>(slot.value());
        if (!stream.Send(sizeof(uint8_t), &rawSlot))
        {
            EterBase::ModernLogger::Error("SendShopSellNew: Failed to send slot.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        if (!stream.Send(sizeof(uint8_t), &count))
        {
            EterBase::ModernLogger::Error("SendShopSellNew: Failed to send count.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug("SendShopSellNew(slot={}, count={})", rawSlot, count);

        return {};
    }
}
