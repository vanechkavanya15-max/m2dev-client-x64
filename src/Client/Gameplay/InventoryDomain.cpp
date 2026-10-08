#include "InventoryDomain.h"

namespace Client::Gameplay {

InventoryDomain::InventoryDomain()
{
    m_mainInventory.resize(INVENTORY_MAX_NUM);
    m_beltInventory.resize(BELT_INVENTORY_MAX_NUM);
    m_equipment.resize(EQUIPMENT_MAX_NUM);
    m_dragonSoulInventory.resize(DRAGON_SOUL_INVENTORY_MAX_NUM);
    m_safeBox.resize(SAFEBOX_MAX_NUM);
}

std::expected<std::reference_wrapper<std::vector<std::optional<ItemData>>>, EterBase::InventoryError> InventoryDomain::GetWindowSlots(uint8_t windowType)
{
    switch (windowType)
    {
        case INVENTORY: return m_mainInventory;
        case BELT_INVENTORY: return m_beltInventory;
        case EQUIPMENT: return m_equipment;
        case DRAGON_SOUL_INVENTORY: return m_dragonSoulInventory;
        case SAFEBOX: return m_safeBox;
        default: return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
}

std::expected<std::reference_wrapper<const std::vector<std::optional<ItemData>>>, EterBase::InventoryError> InventoryDomain::GetWindowSlots(uint8_t windowType) const
{
    switch (windowType)
    {
        case INVENTORY: return m_mainInventory;
        case BELT_INVENTORY: return m_beltInventory;
        case EQUIPMENT: return m_equipment;
        case DRAGON_SOUL_INVENTORY: return m_dragonSoulInventory;
        case SAFEBOX: return m_safeBox;
        default: return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
}

bool InventoryDomain::IsValidCell(uint8_t windowType, EterBase::ItemSlot slot, ItemSize size) const
{
    auto slotsRes = GetWindowSlots(windowType);
    if (!slotsRes) return false;
    const auto& slots = slotsRes.value().get();

    if (windowType == EQUIPMENT || windowType == BELT_INVENTORY || windowType == DRAGON_SOUL_INVENTORY) {
        size = {1, 1}; // Non-grid windows always treat items as 1x1
    }
    
    for (uint8_t y = 0; y < size.height; ++y)
    {
        for (uint8_t x = 0; x < size.width; ++x)
        {
            uint16_t checkSlot = slot.get() + y * INVENTORY_PAGE_WIDTH + x;
            
            if (checkSlot >= slots.size())
            {
                return false;
            }
            
            if (windowType == INVENTORY || windowType == SAFEBOX)
            {
                uint16_t currentColumn = (slot.get() + x) % INVENTORY_PAGE_WIDTH;
                uint16_t startColumn = slot.get() % INVENTORY_PAGE_WIDTH;
                if (currentColumn < startColumn)
                {
                     return false; // Wrapped around row
                }
                
                if (windowType == INVENTORY || windowType == SAFEBOX)
                {
                     uint16_t currentPage = slot.get() / INVENTORY_PAGE_SIZE;
                     uint16_t checkPage = checkSlot / INVENTORY_PAGE_SIZE;
                     if (currentPage != checkPage)
                     {
                         return false; // Crossed page boundary
                     }
                }
            }
        }
    }
    return true;
}

bool InventoryDomain::IsEmpty(uint8_t windowType, EterBase::ItemSlot slot, ItemSize size, std::optional<EterBase::ItemSlot> ignoreSlot) const
{
    auto slotsRes = GetWindowSlots(windowType);
    if (!slotsRes) return false;
    const auto& slots = slotsRes.value().get();

    if (windowType == EQUIPMENT || windowType == BELT_INVENTORY || windowType == DRAGON_SOUL_INVENTORY) {
        size = {1, 1};
    }
    
    for (uint8_t y = 0; y < size.height; ++y)
    {
        for (uint8_t x = 0; x < size.width; ++x)
        {
            uint16_t checkSlot = slot.get() + y * INVENTORY_PAGE_WIDTH + x;
            if (ignoreSlot && checkSlot == ignoreSlot->get()) continue;
            
            if (checkSlot < slots.size() && slots[checkSlot].has_value())
            {
                return false;
            }
        }
    }
    return true;
}

std::expected<void, EterBase::InventoryError> InventoryDomain::SetItem(uint8_t windowType, EterBase::ItemSlot slot, const ItemData& item)
{
    auto slotsRes = GetWindowSlots(windowType);
    if (!slotsRes)
    {
        return std::unexpected(slotsRes.error());
    }
    auto& slots = slotsRes.value().get();
    
    if (slot.get() >= slots.size())
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }

    if (!IsValidCell(windowType, slot, item.size))
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }

    if (!IsEmpty(windowType, slot, item.size))
    {
        return std::unexpected(EterBase::InventoryError::SlotOccupied);
    }

    slots[slot.get()] = item;
    
    ItemSize actualSize = item.size;
    if (windowType == EQUIPMENT || windowType == BELT_INVENTORY || windowType == DRAGON_SOUL_INVENTORY) {
        actualSize = {1, 1};
    }
    
    for (uint8_t y = 0; y < actualSize.height; ++y)
    {
        for (uint8_t x = 0; x < actualSize.width; ++x)
        {
             if (x == 0 && y == 0) continue;
             uint16_t extendedSlot = slot.get() + y * INVENTORY_PAGE_WIDTH + x;
             ItemData extensionData = item;
             extensionData.vnum = EterBase::ItemVnum(0); // Mark as extension
             slots[extendedSlot] = extensionData;
        }
    }

    UserInterface::Core::EventBus::GetInstance().Publish(InventorySlotUpdatedEvent(windowType, slot));
    return {};
}

std::expected<void, EterBase::InventoryError> InventoryDomain::RemoveItem(uint8_t windowType, EterBase::ItemSlot slot)
{
    auto slotsRes = GetWindowSlots(windowType);
    if (!slotsRes)
    {
        return std::unexpected(slotsRes.error());
    }
    auto& slots = slotsRes.value().get();

    if (slot.get() >= slots.size())
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }

    if (!slots[slot.get()].has_value() || slots[slot.get()]->vnum.get() == 0)
    {
        return std::unexpected(EterBase::InventoryError::SlotEmpty);
    }

    ItemSize actualSize = slots[slot.get()]->size;
    if (windowType == EQUIPMENT || windowType == BELT_INVENTORY || windowType == DRAGON_SOUL_INVENTORY) {
        actualSize = {1, 1};
    }
    
    slots[slot.get()] = std::nullopt;

    for (uint8_t y = 0; y < actualSize.height; ++y)
    {
        for (uint8_t x = 0; x < actualSize.width; ++x)
        {
             if (x == 0 && y == 0) continue;
             uint16_t extendedSlot = slot.get() + y * INVENTORY_PAGE_WIDTH + x;
             slots[extendedSlot] = std::nullopt;
        }
    }

    UserInterface::Core::EventBus::GetInstance().Publish(InventorySlotUpdatedEvent(windowType, slot));
    return {};
}

std::expected<ItemData, EterBase::InventoryError> InventoryDomain::GetItem(uint8_t windowType, EterBase::ItemSlot slot) const
{
    auto slotsRes = GetWindowSlots(windowType);
    if (!slotsRes)
    {
        return std::unexpected(slotsRes.error());
    }
    const auto& slots = slotsRes.value().get();

    if (slot.get() >= slots.size())
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }

    if (!slots[slot.get()].has_value() || slots[slot.get()]->vnum.get() == 0)
    {
        return std::unexpected(EterBase::InventoryError::SlotEmpty);
    }

    return slots[slot.get()].value();
}

std::expected<void, EterBase::InventoryError> InventoryDomain::SwapItem(uint8_t windowType, EterBase::ItemSlot srcSlot, uint8_t dstWindowType, EterBase::ItemSlot dstSlot)
{
    auto srcItemRes = GetItem(windowType, srcSlot);
    if (!srcItemRes)
    {
        return std::unexpected(srcItemRes.error());
    }
    
    ItemData srcItem = srcItemRes.value();
    
    auto dstItemRes = GetItem(dstWindowType, dstSlot);
    
    if (dstItemRes)
    {
        // Destination has item, check if we can swap
        ItemData dstItem = dstItemRes.value();
        
        // Ensure no overlap using actual grid check: Temporarily set both to empty, then try validating.
        // Wait, just removing them temporarily works if we check both carefully.
        
        // Internal clean removal without firing events
        auto& srcSlots = GetWindowSlots(windowType).value().get();
        auto& dstSlots = GetWindowSlots(dstWindowType).value().get();
        
        // Remove src from grid temporarily
        ItemSize srcActualSize = srcItem.size;
        if (windowType == EQUIPMENT || windowType == BELT_INVENTORY || windowType == DRAGON_SOUL_INVENTORY) srcActualSize = {1, 1};
        for (uint8_t y = 0; y < srcActualSize.height; ++y) {
            for (uint8_t x = 0; x < srcActualSize.width; ++x) {
                srcSlots[srcSlot.get() + y * INVENTORY_PAGE_WIDTH + x] = std::nullopt;
            }
        }
        
        // Remove dst from grid temporarily
        ItemSize dstActualSize = dstItem.size;
        if (dstWindowType == EQUIPMENT || dstWindowType == BELT_INVENTORY || dstWindowType == DRAGON_SOUL_INVENTORY) dstActualSize = {1, 1};
        for (uint8_t y = 0; y < dstActualSize.height; ++y) {
            for (uint8_t x = 0; x < dstActualSize.width; ++x) {
                dstSlots[dstSlot.get() + y * INVENTORY_PAGE_WIDTH + x] = std::nullopt;
            }
        }
        
        // Check if both places are now clear for their NEW sizes
        bool srcFits = IsValidCell(dstWindowType, dstSlot, srcItem.size) && IsEmpty(dstWindowType, dstSlot, srcItem.size);
        bool dstFits = IsValidCell(windowType, srcSlot, dstItem.size) && IsEmpty(windowType, srcSlot, dstItem.size);

        // Put them back initially
        for (uint8_t y = 0; y < srcActualSize.height; ++y) {
            for (uint8_t x = 0; x < srcActualSize.width; ++x) {
                ItemData ext = srcItem;
                ext.vnum = (x == 0 && y == 0) ? srcItem.vnum : EterBase::ItemVnum(0);
                srcSlots[srcSlot.get() + y * INVENTORY_PAGE_WIDTH + x] = ext;
            }
        }
        for (uint8_t y = 0; y < dstActualSize.height; ++y) {
            for (uint8_t x = 0; x < dstActualSize.width; ++x) {
                ItemData ext = dstItem;
                ext.vnum = (x == 0 && y == 0) ? dstItem.vnum : EterBase::ItemVnum(0);
                dstSlots[dstSlot.get() + y * INVENTORY_PAGE_WIDTH + x] = ext;
            }
        }

        if (srcFits && dstFits) {
             auto res1 = RemoveItem(windowType, srcSlot);
             auto res2 = RemoveItem(dstWindowType, dstSlot);
             auto res3 = SetItem(dstWindowType, dstSlot, srcItem);
             auto res4 = SetItem(windowType, srcSlot, dstItem);
             
             if (!res1 || !res2 || !res3 || !res4) {
                 EterBase::ModernLogger::Error("Critical swap failure despite pre-checks. Possible state corruption.");
                 return std::unexpected(EterBase::InventoryError::SlotOccupied);
             }
             return {};
        }
        else {
             return std::unexpected(EterBase::InventoryError::SlotOccupied);
        }
    }
    else
    {
        // Destination is empty
        if (IsValidCell(dstWindowType, dstSlot, srcItem.size) && IsEmpty(dstWindowType, dstSlot, srcItem.size))
        {
             RemoveItem(windowType, srcSlot);
             SetItem(dstWindowType, dstSlot, srcItem);
             return {};
        }
        else
        {
             return std::unexpected(EterBase::InventoryError::SlotOccupied);
        }
    }
}

std::expected<void, EterBase::InventoryError> InventoryDomain::SplitItem(uint8_t windowType, EterBase::ItemSlot srcSlot, EterBase::ItemSlot dstSlot, uint32_t splitCount)
{
    if (splitCount == 0)
    {
        return std::unexpected(EterBase::InventoryError::InsufficientCount);
    }

    auto srcItemRes = GetItem(windowType, srcSlot);
    if (!srcItemRes)
    {
        return std::unexpected(srcItemRes.error());
    }

    ItemData srcItem = srcItemRes.value();
    if (srcItem.count <= splitCount)
    {
        return std::unexpected(EterBase::InventoryError::InsufficientCount);
    }

    if (!IsValidCell(windowType, dstSlot, srcItem.size) || !IsEmpty(windowType, dstSlot, srcItem.size))
    {
         return std::unexpected(EterBase::InventoryError::SlotOccupied);
    }

    // Update src item count
    RemoveItem(windowType, srcSlot);
    srcItem.count -= splitCount;
    SetItem(windowType, srcSlot, srcItem);

    // Create split item
    ItemData splitItem = srcItem;
    splitItem.count = splitCount;
    SetItem(windowType, dstSlot, splitItem);

    return {};
}

} // namespace Client::Gameplay
