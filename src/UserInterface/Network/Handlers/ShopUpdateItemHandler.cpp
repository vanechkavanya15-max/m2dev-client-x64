#include "../../StdAfx.h"
#include "ShopUpdateItemHandler.h"
#include "../../Packet.h"
#include "../../PythonShop.h"
#include "../../../EterBase/LogModern.h"

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessShopUpdateItem(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCShopUpdateItem))
        {
            EterBase::ModernLogger::Error("ProcessShopUpdateItem: Buffer underflow (size: {})", buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCShopUpdateItem*>(buffer.data());
        
        const EterBase::ItemSlot position(packet->pos);

        // Update the CPythonShop instance memory state
        CPythonShop::Instance().SetItemData(position.value(), packet->item);

        EterBase::ModernLogger::Info("ProcessShopUpdateItem: Successfully updated item at slot {}", position);

        // Trigger the event for UI refresh (event-driven decoupled design)
        UserInterface::Core::EventBus::GetInstance().Publish(ShopItemUpdatedEvent(position));

        return {};
    }

    bool HandleShopUpdateItem(std::span<const uint8_t> buffer)
    {
        return ProcessShopUpdateItem(buffer).has_value();
    }
}
