#include "../../StdAfx.h"
#include "ItemUseModernHandler.h"
#include "../../../EterBase/LogModern.h"

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessItemUsePacket(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketItemUse))
        {
            EterBase::ModernLogger::Error("ProcessItemUsePacket: Buffer underflow. Expected >= {} bytes, got {}", 
                sizeof(PacketItemUse), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketItemUse*>(buffer.data());

        // Deklarowanie jako silne typy, by uchronic domene
        const EterBase::EntityId characterId(packet->ch_vid);
        const EterBase::EntityId targetId(packet->victim_vid);
        const EterBase::ItemVnum itemVnum(packet->vnum);
        const EterBase::ItemSlot itemCell(packet->cell);

        EterBase::ModernLogger::Debug(
            "ProcessItemUsePacket: Uzyto przedmiotu! VNUM: {}, Slot: {}, ChID: {}, VictimID: {}", 
            itemVnum.value(), itemCell.value(), characterId.value(), targetId.value());

        // Emitowanie zdarzenia w EventBus, oddzielajace warstwe sieciowa od UI / CPythonPlayer
        UserInterface::Core::EventBus::GetInstance().Publish(InventoryRefreshEvent{});

        return {}; // Sukces - zwraca std::expected ze statusem void
    }
}
