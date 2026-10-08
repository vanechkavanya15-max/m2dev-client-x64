#include "StdAfx.h"
#include "InventoryCommandEncoder.h"
#include "../../UserInterface/Packet.h"
#include <cstring>

namespace Client::Network {

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> InventoryCommandEncoder::EncodeUseItem(EterBase::ItemSlot slot)
    {
        TPacketCGItemUse packet{};
        packet.header = CG::ITEM_USE;
        packet.length = sizeof(TPacketCGItemUse);
        packet.pos = TItemPos(INVENTORY, slot.value());

        std::vector<uint8_t> buffer(sizeof(TPacketCGItemUse));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGItemUse));

        return buffer;
    }

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> InventoryCommandEncoder::EncodeDropItem(EterBase::ItemSlot slot, uint32_t gold, uint8_t count)
    {
        TPacketCGItemDrop2 packet{};
        packet.header = CG::ITEM_DROP2;
        packet.length = sizeof(TPacketCGItemDrop2);
        packet.pos = TItemPos(INVENTORY, slot.value());
        packet.gold = gold;
        packet.count = count;

        std::vector<uint8_t> buffer(sizeof(TPacketCGItemDrop2));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGItemDrop2));

        return buffer;
    }

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> InventoryCommandEncoder::EncodePickupItem(EterBase::EntityId itemVid)
    {
        TPacketCGItemPickUp packet{};
        packet.header = CG::ITEM_PICKUP;
        packet.length = sizeof(TPacketCGItemPickUp);
        packet.vid = itemVid.value();

        std::vector<uint8_t> buffer(sizeof(TPacketCGItemPickUp));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGItemPickUp));

        return buffer;
    }

} // namespace Client::Network
