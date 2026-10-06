#include "../StdAfx.h"
#include "InventoryGridValidator.h"
#include "../../GameLib/ItemManager.h"
#include "../../GameLib/ItemData.h"
#include "../../EterBase/ModernLogger.h"
#include "../Core/EventBus.h"

namespace UserInterface::Services
{
    EterBase::PacketResult<void> InventoryGridValidator::ValidateItemPlacement(
        const IInventoryService& inventoryService,
        EterBase::ItemSlot slot,
        EterBase::ItemVnum vnum)
    {
        CItemData* itemData = nullptr;
        if (!CItemManager::Instance().GetItemDataPointer(vnum.value(), &itemData))
        {
            EterBase::ModernLogger::Error("InventoryGridValidator: Invalid VNUM {}", vnum.value());
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        uint8_t itemSize = itemData->GetSize();
        if (itemSize == 0)
        {
            itemSize = 1;
        }

        if (!CanPlaceItemSize(inventoryService, slot, itemSize))
        {
            EterBase::ModernLogger::Error("InventoryGridValidator: Cannot place item of size {} at slot {}", itemSize, slot.value());
            
            ValidationFailureEvent event;
            event.slot = slot;
            event.vnum = vnum;
            UserInterface::Core::EventBus::GetInstance().Publish(event);
            
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        return {};
    }

    bool InventoryGridValidator::CanPlaceItemSize(
        const IInventoryService& inventoryService,
        EterBase::ItemSlot slot,
        uint8_t itemSize)
    {
        if (itemSize == 0)
        {
            itemSize = 1;
        }

        uint16_t slotIndex = slot.value();

        const uint16_t INVENTORY_PAGE_SIZE = 45; // 5x9 per page
        const uint16_t INVENTORY_WIDTH = 5;

        uint16_t pageSlotIndex = slotIndex % INVENTORY_PAGE_SIZE;

        for (uint8_t i = 0; i < itemSize; ++i)
        {
            if (pageSlotIndex + (i * INVENTORY_WIDTH) >= INVENTORY_PAGE_SIZE)
            {
                return false;
            }

            EterBase::ItemSlot checkSlot(slotIndex + (i * INVENTORY_WIDTH));
            
            if (!inventoryService.IsSlotEmpty(checkSlot))
            {
                return false;
            }
        }

        return true;
    }
}
