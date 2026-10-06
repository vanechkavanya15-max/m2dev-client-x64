#include "StdAfx.h"
/**
 * @file SendExchangeItemPacket.cpp
 * @brief Modern C++23 implementation for sending the exchange item add packet.
 */

#include "SendExchangeItemPacket.h"
#include "../../Packet.h"
#include "../../../EterBase/LogModern.h"
#include <cstring>

namespace Network
{
    /**
     * @brief Formats and sends an exchange item add packet to the server.
     * 
     * @param itemPos The inventory or belt position of the item to offer.
     * @param displayPos The target window slot index in the exchange UI.
     * @param sendCallback A callback function taking a span of bytes to send over the network.
     * @return EterBase::PacketResult<void> Returns success or a specific PacketError on failure.
     */
    EterBase::PacketResult<void> SendExchangeItemAddPacket(
        const TItemPos& itemPos, 
        EterBase::ItemSlot displayPos, 
        const std::function<bool(std::span<const uint8_t>)>& sendCallback)
    {
        if (!sendCallback)
        {
            EterBase::ModernLogger::Error("[Network] SendExchangeItemAddPacket: callback is null.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        // Validate the source item cell. We use const_cast due to legacy IsValidCell implementation.
        if (!const_cast<TItemPos*>(&itemPos)->IsValidCell())
        {
            EterBase::ModernLogger::Warn("[Network] SendExchangeItemAddPacket: Invalid item cell specified. Window: {}, Cell: {}", 
                itemPos.window_type, itemPos.cell);
            // This is effectively an invalid action by the client.
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }


        TPacketCGExchange packet;


        std::memset(&packet, 0, sizeof(packet));

        packet.header = CG::EXCHANGE;
        packet.length = sizeof(packet);
        packet.subheader = ExchangeSub::CG::ITEM_ADD;
        
        // We do not set arg1 for ITEM_ADD, it is used for ELK. 
        packet.arg1 = 0;
        
        // The display position goes into arg2.
        packet.arg2 = static_cast<uint8_t>(displayPos.value());
        
        // The source inventory position.
        packet.Pos = itemPos;

        std::span<const uint8_t> packetSpan(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));

        if (!sendCallback(packetSpan))
        {
            EterBase::ModernLogger::Error("[Network] SendExchangeItemAddPacket: failed to send packet payload.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug("[Network] Sent Exchange Item Add (Window: {}, Cell: {}, DisplaySlot: {})",
            itemPos.window_type, itemPos.cell, displayPos.value());

        // Publish event to EventBus for decoupled UI/state tracking.
        UserInterface::Core::ExchangeItemOfferEvent eventData{itemPos, displayPos};
        UserInterface::Core::EventBus::GetInstance().Publish(eventData);

        return {};
    }
}
