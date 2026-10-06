#include "../../StdAfx.h"
#include "SendItemDropPacket.h"
#include "../../../EterLib/NetStream.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include "../../Packet.h"
#include <span>

#pragma pack(push, 1)

/**
 * @brief Proxy structure representing the legacy item drop packet.
 * 
 * Ensures strict 1-byte alignment to match network protocol specifications.
 */
struct ProxyPacketCGItemDrop
{
    uint16_t header;    ///< Packet header identifier.
    uint16_t length;    ///< Length of the packet.
    TItemPos pos;       ///< Position of the item to drop.
    uint32_t elk;       ///< Elk (gold) to drop.
};

/**
 * @brief Proxy structure representing the new item drop packet with specific count.
 * 
 * Ensures strict 1-byte alignment to match network protocol specifications.
 */
struct ProxyPacketCGItemDrop2
{
    uint16_t header;    ///< Packet header identifier.
    uint16_t length;    ///< Length of the packet.
    TItemPos pos;       ///< Position of the item to drop.
    uint32_t gold;      ///< Gold to drop.
    uint8_t  count;     ///< Count of the item to drop.
};

#pragma pack(pop)

namespace UserInterface::Network::Senders
{
    /**
     * @brief Event triggered when an item drop packet is successfully sent.
     */
    struct ItemDropPacketSentEvent : public Core::IEvent {
        uint8_t windowType;
        EterBase::ItemSlot slot;
        uint32_t gold;
        uint8_t count;

        ItemDropPacketSentEvent(uint8_t windowType, EterBase::ItemSlot slot, uint32_t gold, uint8_t count)
            : windowType(windowType), slot(slot), gold(gold), count(count) {}
    };

    EterBase::PacketResult<void> SendItemDropPacket::SendItemDrop(uint8_t windowType, EterBase::ItemSlot itemSlot, uint32_t elk, CNetworkStream* networkStream)
    {
        if (!networkStream)
        {
            EterBase::ModernLogger::Error("SendItemDropPacket::SendItemDrop: networkStream is null.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        ProxyPacketCGItemDrop dropPacket{};
        dropPacket.header = CG::ITEM_DROP;
        dropPacket.length = sizeof(dropPacket);
        dropPacket.pos = TItemPos(windowType, itemSlot.value());
        dropPacket.elk = elk;

        std::span<const uint8_t> buffer(reinterpret_cast<const uint8_t*>(&dropPacket), sizeof(dropPacket));
        if (!networkStream->Send(static_cast<int>(buffer.size()), buffer.data()))
        {
            EterBase::ModernLogger::Error("SendItemDropPacket::SendItemDrop: Failed to send item drop packet.");
            return EterBase::MakeError(EterBase::PacketError::SequenceMismatch); // Generic network transmission error
        }

        EterBase::ModernLogger::Debug("SendItemDropPacket::SendItemDrop: Dropped item from window={}, cell={}, elk={}", 
            windowType, itemSlot.value(), elk);

        // Publish event for UI decoupling
        Core::EventBus::GetInstance().Publish(ItemDropPacketSentEvent(windowType, itemSlot, elk, 0));

        return {};
    }

    EterBase::PacketResult<void> SendItemDropPacket::SendItemDropNew(uint8_t windowType, EterBase::ItemSlot itemSlot, uint32_t gold, uint8_t count, CNetworkStream* networkStream)
    {
        if (!networkStream)
        {
            EterBase::ModernLogger::Error("SendItemDropPacket::SendItemDropNew: networkStream is null.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        ProxyPacketCGItemDrop2 dropPacket{};
        dropPacket.header = CG::ITEM_DROP2;
        dropPacket.length = sizeof(dropPacket);
        dropPacket.pos = TItemPos(windowType, itemSlot.value());
        dropPacket.gold = gold;
        dropPacket.count = count;

        std::span<const uint8_t> buffer(reinterpret_cast<const uint8_t*>(&dropPacket), sizeof(dropPacket));
        if (!networkStream->Send(static_cast<int>(buffer.size()), buffer.data()))
        {
            EterBase::ModernLogger::Error("SendItemDropPacket::SendItemDropNew: Failed to send item drop packet (count).");
            return EterBase::MakeError(EterBase::PacketError::SequenceMismatch); // Generic network transmission error
        }

        EterBase::ModernLogger::Debug("SendItemDropPacket::SendItemDropNew: Dropped item from window={}, cell={}, gold={}, count={}", 
            windowType, itemSlot.value(), gold, count);

        // Publish event for UI decoupling
        Core::EventBus::GetInstance().Publish(ItemDropPacketSentEvent(windowType, itemSlot, gold, count));

        return {};
    }
}
