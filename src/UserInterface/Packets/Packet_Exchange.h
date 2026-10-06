/**
 * @file Packet_Exchange.h
 * @brief Modern C++20 declarations for the Exchange (Trading) network packets.
 * 
 * This file contains strictly decoupled structures for the player-to-player
 * trading system, avoiding any direct GUI calls. All packet structs are
 * aligned to 1-byte boundaries for network serialization.
 */

#pragma once

#include <cstdint>
#include "../GameType.h"

#pragma pack(push, 1)

/**
 * @brief Represents an exchange command packet sent from Client to Game Server.
 * 
 * Used for interactions within the exchange window, such as adding an item,
 * removing an item, adding currency (yang/elk), or accepting the trade.
 */
struct TPacketCGExchange
{
    uint16_t header;       ///< Packet header identifier (CG::EXCHANGE)
    uint16_t length;       ///< Total length of the packet in bytes
    uint8_t  subheader;    ///< Specific exchange action (ExchangeSub::CG::...)
    uint32_t value1;       ///< Primary argument (e.g., currency amount, item index)
    uint8_t  value2;       ///< Secondary argument (e.g., target window slot index)
    TItemPos itemPos;      ///< The source position of the item in inventory/belt
};

/**
 * @brief Represents an exchange state update packet sent from Game Server to Client.
 * 
 * Used to notify the UI about the current state of the trade, including
 * item additions, currency additions, and readiness state of both parties.
 */
struct TPacketGCExchange
{
    uint16_t header;       ///< Packet header identifier (GC::EXCHANGE)
    uint16_t length;       ///< Total length of the packet in bytes
    uint8_t  subheader;    ///< Specific exchange notification (ExchangeSub::GC::...)
    uint8_t  isInitiator;  ///< Non-zero if the packet targets the client's own exchange window side
    uint32_t value1;       ///< Primary argument (e.g., item VNUM or currency amount)
    TItemPos itemPos;      ///< Target position in the exchange window
    uint32_t count;        ///< Amount or count of the item
    int32_t  sockets[ITEM_SOCKET_SLOT_MAX_NUM];                ///< Metin stones/sockets applied to the item
    TPlayerItemAttribute attributes[ITEM_ATTRIBUTE_SLOT_MAX_NUM]; ///< Attributes/Bonuses applied to the item
};

#pragma pack(pop)
