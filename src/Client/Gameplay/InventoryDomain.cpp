#include "InventoryDomain.h"
#include "Client/Core/EventBus.h"
#include <algorithm>

namespace Client::Gameplay {

InventoryDomain::InventoryDomain()
{
    m_mainInventory.resize(INVENTORY_MAX_NUM);
    m_beltInventory.resize(BELT_INVENTORY_MAX_NUM);
    m_equipment.resize(EQUIPMENT_MAX_NUM);
    m_dragonSoulInventory.resize(DRAGON_SOUL_INVENTORY_MAX_NUM);
    m_safeBox.resize(SAFEBOX_MAX_NUM);
}

std::expected<std::reference_wrapper<std::vector<std::optional<ItemData>>>, EterBase::InventoryError> InventoryDomain::GetWindowSlots(InventoryWindow windowType)
{
    switch (windowType)
    {
        case InventoryWindow::Inventory: return m_mainInventory;
        case InventoryWindow::Belt: return m_beltInventory;
        case InventoryWindow::Equipment: return m_equipment;
        case InventoryWindow::DragonSoul: return m_dragonSoulInventory;
        case InventoryWindow::SafeBox: return m_safeBox;
        default: return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
}

std::expected<std::reference_wrapper<const std::vector<std::optional<ItemData>>>, EterBase::InventoryError> InventoryDomain::GetWindowSlots(InventoryWindow windowType) const
{
    switch (windowType)
    {
        case InventoryWindow::Inventory: return m_mainInventory;
        case InventoryWindow::Belt: return m_beltInventory;
        case InventoryWindow::Equipment: return m_equipment;
        case InventoryWindow::DragonSoul: return m_dragonSoulInventory;
        case InventoryWindow::SafeBox: return m_safeBox;
        default: return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
}

bool InventoryDomain::IsValidCell(InventoryWindow windowType, EterBase::ItemSlot slot, ItemSize size) const
{
    auto slotsRes = GetWindowSlots(windowType);
    if (!slotsRes) return false;
    const auto& slots = slotsRes.value().get();

    if (windowType == InventoryWindow::Equipment || windowType == InventoryWindow::Belt || windowType == InventoryWindow::DragonSoul) {
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
            
            if (windowType == InventoryWindow::Inventory || windowType == InventoryWindow::SafeBox)
            {
                uint16_t currentColumn = (slot.get() + x) % INVENTORY_PAGE_WIDTH;
                uint16_t startColumn = slot.get() % INVENTORY_PAGE_WIDTH;
                if (currentColumn < startColumn)
                {
                     return false; // Wrapped around row
                }
                
                uint16_t currentPage = slot.get() / INVENTORY_PAGE_SIZE;
                uint16_t checkPage = checkSlot / INVENTORY_PAGE_SIZE;
                if (currentPage != checkPage)
                {
                    return false; // Crossed page boundary
                }
            }
        }
    }
    return true;
}

bool InventoryDomain::IsEmpty(InventoryWindow windowType, EterBase::ItemSlot slot, ItemSize size, std::optional<EterBase::ItemSlot> ignoreSlot) const
{
    auto slotsRes = GetWindowSlots(windowType);
    if (!slotsRes) return false;
    const auto& slots = slotsRes.value().get();

    if (windowType == InventoryWindow::Equipment || windowType == InventoryWindow::Belt || windowType == InventoryWindow::DragonSoul) {
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

int32_t InventoryDomain::FindEmptyCell() const
{
    return FindEmptyCell(InventoryWindow::Inventory, ItemSize{1, 1});
}

int32_t InventoryDomain::FindEmptyCell(ItemSize size) const
{
    return FindEmptyCell(InventoryWindow::Inventory, size);
}

int32_t InventoryDomain::FindEmptyCell(InventoryWindow window, ItemSize size) const
{
    auto slotsRes = GetWindowSlots(window);
    if (!slotsRes) return -1;
    const auto& slots = slotsRes.value().get();

    for (size_t i = 0; i < slots.size(); ++i)
    {
        EterBase::ItemSlot slot(static_cast<uint16_t>(i));
        if (IsValidCell(window, slot, size) && IsEmpty(window, slot, size))
        {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

void InventoryDomain::Clear()
{
    std::fill(m_mainInventory.begin(), m_mainInventory.end(), std::nullopt);
    std::fill(m_beltInventory.begin(), m_beltInventory.end(), std::nullopt);
    std::fill(m_equipment.begin(), m_equipment.end(), std::nullopt);
    std::fill(m_dragonSoulInventory.begin(), m_dragonSoulInventory.end(), std::nullopt);
    std::fill(m_safeBox.begin(), m_safeBox.end(), std::nullopt);
}

void InventoryDomain::ClearWindow(InventoryWindow window)
{
    auto slotsRes = GetWindowSlots(window);
    if (slotsRes)
    {
        auto& slots = slotsRes.value().get();
        std::fill(slots.begin(), slots.end(), std::nullopt);
    }
}

void InventoryDomain::NotifySlotUpdated(InventoryWindow windowType, EterBase::ItemSlot slot)
{
    if (m_slotUpdateCallback)
    {
        m_slotUpdateCallback(InventorySlotUpdatedEvent(windowType, slot));
    }

    uint32_t vnum = 0;
    uint32_t count = 0;
    auto itemRes = GetItem(windowType, slot);
    if (itemRes.has_value())
    {
        vnum = itemRes.value().vnum.get();
        count = itemRes.value().count;
    }
    ::UserInterface::Core::EventBus::GetInstance().Publish(
        ::Client::Core::InventorySlotUpdatedEvent{ slot.get(), vnum, count }
    );
}

std::expected<void, EterBase::InventoryError> InventoryDomain::SetItem(InventoryWindow windowType, EterBase::ItemSlot slot, const ItemData& item)
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
    if (windowType == InventoryWindow::Equipment || windowType == InventoryWindow::Belt || windowType == InventoryWindow::DragonSoul) {
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

    NotifySlotUpdated(windowType, slot);
    return {};
}

std::expected<void, EterBase::InventoryError> InventoryDomain::RemoveItem(InventoryWindow windowType, EterBase::ItemSlot slot)
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
    if (windowType == InventoryWindow::Equipment || windowType == InventoryWindow::Belt || windowType == InventoryWindow::DragonSoul) {
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

    NotifySlotUpdated(windowType, slot);
    return {};
}

std::expected<ItemData, EterBase::InventoryError> InventoryDomain::GetItem(InventoryWindow windowType, EterBase::ItemSlot slot) const
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

std::expected<void, EterBase::InventoryError> InventoryDomain::SwapItem(InventoryWindow windowType, EterBase::ItemSlot srcSlot, InventoryWindow dstWindowType, EterBase::ItemSlot dstSlot)
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
        
        auto& srcSlots = GetWindowSlots(windowType).value().get();
        auto& dstSlots = GetWindowSlots(dstWindowType).value().get();
        
        // Remove src from grid temporarily
        ItemSize srcActualSize = srcItem.size;
        if (windowType == InventoryWindow::Equipment || windowType == InventoryWindow::Belt || windowType == InventoryWindow::DragonSoul) srcActualSize = {1, 1};
        for (uint8_t y = 0; y < srcActualSize.height; ++y) {
            for (uint8_t x = 0; x < srcActualSize.width; ++x) {
                srcSlots[srcSlot.get() + y * INVENTORY_PAGE_WIDTH + x] = std::nullopt;
            }
        }
        
        // Remove dst from grid temporarily
        ItemSize dstActualSize = dstItem.size;
        if (dstWindowType == InventoryWindow::Equipment || dstWindowType == InventoryWindow::Belt || dstWindowType == InventoryWindow::DragonSoul) dstActualSize = {1, 1};
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
             auto resRem = RemoveItem(windowType, srcSlot);
             if (!resRem) return std::unexpected(resRem.error());
             auto resSet = SetItem(dstWindowType, dstSlot, srcItem);
             if (!resSet) return std::unexpected(resSet.error());
             return {};
        }
        else
        {
             return std::unexpected(EterBase::InventoryError::SlotOccupied);
        }
    }
}

std::expected<void, EterBase::InventoryError> InventoryDomain::SplitItem(InventoryWindow windowType, EterBase::ItemSlot srcSlot, EterBase::ItemSlot dstSlot, uint32_t splitCount)
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
    auto resRemove = RemoveItem(windowType, srcSlot);
    if (!resRemove) return std::unexpected(resRemove.error());

    srcItem.count -= splitCount;
    auto resSetSrc = SetItem(windowType, srcSlot, srcItem);
    if (!resSetSrc) return std::unexpected(resSetSrc.error());

    // Create split item
    ItemData splitItem = srcItem;
    splitItem.count = splitCount;
    auto resSetDst = SetItem(windowType, dstSlot, splitItem);
    if (!resSetDst) return std::unexpected(resSetDst.error());

    return {};
}

namespace {
    Client::Core::InventoryError MapToDomainError(EterBase::InventoryError err) {
        switch (err) {
            case EterBase::InventoryError::None: return Client::Core::InventoryError::None;
            case EterBase::InventoryError::SlotOutOfRange: return Client::Core::InventoryError::SlotOutOfBounds;
            case EterBase::InventoryError::SlotOccupied: return Client::Core::InventoryError::SlotOccupied;
            case EterBase::InventoryError::SlotEmpty: return Client::Core::InventoryError::SlotEmpty;
            case EterBase::InventoryError::ItemLocked: return Client::Core::InventoryError::ItemLocked;
            case EterBase::InventoryError::InsufficientCount: return Client::Core::InventoryError::InsufficientCount;
            case EterBase::InventoryError::InvalidVnum: return Client::Core::InventoryError::InvalidVnum;
            default: return Client::Core::InventoryError::None;
        }
    }
}

Client::Core::Result<void, Client::Core::InventoryError> InventoryDomain::AddItem(InventoryWindow windowType, EterBase::ItemSlot slot, const ItemData& item)
{
    return SetItemResult(windowType, slot, item);
}

Client::Core::Result<void, Client::Core::InventoryError> InventoryDomain::AddItem(InventoryWindow windowType, const ItemData& item)
{
    if (item.vnum.get() == 0) {
        return std::unexpected(Client::Core::InventoryError::InvalidVnum);
    }
    int32_t freeSlot = FindEmptyCell(windowType, item.size);
    if (freeSlot < 0) {
        return std::unexpected(Client::Core::InventoryError::SlotOutOfBounds);
    }
    return SetItemResult(windowType, EterBase::ItemSlot(static_cast<uint16_t>(freeSlot)), item);
}

Client::Core::Result<void, Client::Core::InventoryError> InventoryDomain::AddItem(const ItemData& item)
{
    return AddItem(InventoryWindow::Inventory, item);
}

Client::Core::Result<void, Client::Core::InventoryError> InventoryDomain::AddItem(uint16_t slot, const ItemData& item)
{
    return SetItemResult(InventoryWindow::Inventory, EterBase::ItemSlot(slot), item);
}

Client::Core::Result<void, Client::Core::InventoryError> InventoryDomain::RemoveItem(uint16_t slot)
{
    return RemoveItemResult(InventoryWindow::Inventory, EterBase::ItemSlot(slot));
}

Client::Core::Result<void, Client::Core::InventoryError> InventoryDomain::RemoveItem(InventoryWindow windowType, uint16_t slot)
{
    return RemoveItemResult(windowType, EterBase::ItemSlot(slot));
}

Client::Core::Result<void, Client::Core::InventoryError> InventoryDomain::RemoveItemResult(EterBase::ItemSlot slot)
{
    return RemoveItemResult(InventoryWindow::Inventory, slot);
}

Client::Core::Result<void, Client::Core::InventoryError> InventoryDomain::RemoveItemResult(uint16_t slot)
{
    return RemoveItemResult(InventoryWindow::Inventory, EterBase::ItemSlot(slot));
}

Client::Core::Result<void, Client::Core::InventoryError> InventoryDomain::SetItemResult(InventoryWindow windowType, EterBase::ItemSlot slot, const ItemData& item)
{
    auto res = SetItem(windowType, slot, item);
    if (!res) {
        return std::unexpected(MapToDomainError(res.error()));
    }
    return {};
}

Client::Core::Result<void, Client::Core::InventoryError> InventoryDomain::RemoveItemResult(InventoryWindow windowType, EterBase::ItemSlot slot)
{
    auto res = RemoveItem(windowType, slot);
    if (!res) {
        return std::unexpected(MapToDomainError(res.error()));
    }
    return {};
}

Client::Core::Result<void, Client::Core::InventoryError> InventoryDomain::SwapItemResult(InventoryWindow windowType, EterBase::ItemSlot srcSlot, InventoryWindow dstWindowType, EterBase::ItemSlot dstSlot)
{
    auto res = SwapItem(windowType, srcSlot, dstWindowType, dstSlot);
    if (!res) {
        return std::unexpected(MapToDomainError(res.error()));
    }
    return {};
}

Client::Core::Result<void, Client::Core::InventoryError> InventoryDomain::SplitItemResult(InventoryWindow windowType, EterBase::ItemSlot srcSlot, EterBase::ItemSlot dstSlot, uint32_t splitCount)
{
    auto res = SplitItem(windowType, srcSlot, dstSlot, splitCount);
    if (!res) {
        return std::unexpected(MapToDomainError(res.error()));
    }
    return {};
}

Client::Core::Result<ItemData, Client::Core::InventoryError> InventoryDomain::GetItemResult(InventoryWindow windowType, EterBase::ItemSlot slot) const
{
    auto res = GetItem(windowType, slot);
    if (!res) {
        return std::unexpected(MapToDomainError(res.error()));
    }
    return res.value();
}

} // namespace Client::Gameplay
