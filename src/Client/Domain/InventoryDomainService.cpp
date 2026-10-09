#include "InventoryDomainService.h"

namespace Client::Domain {

InventoryDomainService::InventoryDomainService(Gameplay::InventoryDomain& inventory) noexcept
    : m_inventory(inventory)
{
}

Core::Result<void, Core::InventoryError> InventoryDomainService::MoveItem(
    Gameplay::InventoryWindow srcWindow, 
    Core::ItemSlot srcSlot,
    Gameplay::InventoryWindow dstWindow, 
    Core::ItemSlot dstSlot) const
{
    if (srcWindow == dstWindow && srcSlot == dstSlot) {
        return {}; // Zero operation
    }

    return m_inventory.SwapItemResult(srcWindow, srcSlot, dstWindow, dstSlot);
}

Core::Result<void, Core::InventoryError> InventoryDomainService::SplitItem(
    Gameplay::InventoryWindow window,
    Core::ItemSlot srcSlot,
    Core::ItemSlot dstSlot,
    uint32_t amount) const
{
    if (amount == 0) {
        return std::unexpected(Core::InventoryError::InsufficientCount);
    }
    if (srcSlot == dstSlot) {
        return std::unexpected(Core::InventoryError::SlotOccupied);
    }

    return m_inventory.SplitItemResult(window, srcSlot, dstSlot, amount);
}

Core::Result<Gameplay::ItemData, Core::InventoryError> InventoryDomainService::DropItem(
    Gameplay::InventoryWindow window,
    Core::ItemSlot slot) const
{
    auto itemResult = m_inventory.GetItemResult(window, slot);
    if (!itemResult) {
        return std::unexpected(itemResult.error());
    }

    auto removeResult = m_inventory.RemoveItemResult(window, slot);
    if (!removeResult) {
        return std::unexpected(removeResult.error());
    }

    return itemResult.value();
}

} // namespace Client::Domain
