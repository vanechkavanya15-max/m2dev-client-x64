#include "SafeboxCommandHandler.h"
#include "../../EterBase/ModernLogger.h"

namespace Client::Gameplay {

SafeboxCommandHandler::SafeboxCommandHandler(InventoryDomain& inventory, SafeBox& safebox)
    : m_inventory(inventory), m_safebox(safebox) {}

EterBase::Result<void, CommandError> SafeboxCommandHandler::ValidatePassword(std::string_view password) {
    if (m_isOpen) {
        EterBase::ModernLogger::Error("SafeboxCommandHandler::ValidatePassword: SafeBox is already opened");
        return std::unexpected(CommandError::AlreadyOpened);
    }

    if (password.empty()) {
        EterBase::ModernLogger::Error("SafeboxCommandHandler::ValidatePassword: Password cannot be empty");
        return std::unexpected(CommandError::InvalidPassword);
    }

    // Since this is a command handler, we just validate password string length > 0
    // Real password validation (hashing, matching with DB) is done on the server.
    // As per the instructions, just implement a basic validation.

    m_isOpen = true;
    EterBase::ModernLogger::Info("SafeboxCommandHandler::ValidatePassword: Password validated successfully, SafeBox opened");
    return {};
}

EterBase::Result<void, CommandError> SafeboxCommandHandler::MoveItemToSafebox(EterBase::ItemSlot inventorySlot, EterBase::ItemSlot safeboxSlot) {
    if (!m_isOpen) {
        EterBase::ModernLogger::Error("SafeboxCommandHandler::MoveItemToSafebox: SafeBox is not opened");
        return std::unexpected(CommandError::NotOpened);
    }

    auto itemRes = m_inventory.GetItem(INVENTORY, inventorySlot);
    if (!itemRes.has_value()) {
        EterBase::ModernLogger::Error("SafeboxCommandHandler::MoveItemToSafebox: Item not found in inventory slot {}", inventorySlot.get());
        return std::unexpected(CommandError::ItemNotFound);
    }

    auto existingSafeBoxItem = m_safebox.GetItem(safeboxSlot);
    if (existingSafeBoxItem.has_value()) {
        EterBase::ModernLogger::Error("SafeboxCommandHandler::MoveItemToSafebox: Target safebox slot {} is already occupied", safeboxSlot.get());
        return std::unexpected(CommandError::SafeboxFull);
    }

    SafeBoxItem safeBoxItem{itemRes->vnum, static_cast<uint8_t>(itemRes->count)};
    auto setRes = m_safebox.SetItem(safeboxSlot, safeBoxItem);
    if (!setRes.has_value()) {
        EterBase::ModernLogger::Error("SafeboxCommandHandler::MoveItemToSafebox: Failed to set item in safebox slot {}", safeboxSlot.get());
        return std::unexpected(CommandError::InvalidSlot);
    }

    auto removeRes = m_inventory.RemoveItem(INVENTORY, inventorySlot);
    if (!removeRes.has_value()) {
        EterBase::ModernLogger::Error("SafeboxCommandHandler::MoveItemToSafebox: Failed to remove item from inventory slot {}", inventorySlot.get());
        // Attempt rollback
        (void)m_safebox.RemoveItem(safeboxSlot);
        return std::unexpected(CommandError::InvalidSlot);
    }

    EterBase::ModernLogger::Info("SafeboxCommandHandler::MoveItemToSafebox: Item moved from inventory {} to safebox {}", inventorySlot.get(), safeboxSlot.get());
    return {};
}

EterBase::Result<void, CommandError> SafeboxCommandHandler::MoveItemToInventory(EterBase::ItemSlot safeboxSlot, EterBase::ItemSlot inventorySlot) {
    if (!m_isOpen) {
        EterBase::ModernLogger::Error("SafeboxCommandHandler::MoveItemToInventory: SafeBox is not opened");
        return std::unexpected(CommandError::NotOpened);
    }

    auto safeBoxItem = m_safebox.GetItem(safeboxSlot);
    if (!safeBoxItem.has_value()) {
        EterBase::ModernLogger::Error("SafeboxCommandHandler::MoveItemToInventory: Item not found in safebox slot {}", safeboxSlot.get());
        return std::unexpected(CommandError::ItemNotFound);
    }

    auto inventoryItemRes = m_inventory.GetItem(INVENTORY, inventorySlot);
    if (inventoryItemRes.has_value()) {
        EterBase::ModernLogger::Error("SafeboxCommandHandler::MoveItemToInventory: Target inventory slot {} is already occupied", inventorySlot.get());
        return std::unexpected(CommandError::InventoryFull);
    }

    ItemData itemData{safeBoxItem->vnum, safeBoxItem->count, {1, 1}}; // Assuming 1x1 size for simplicity if no specific info available
    auto setRes = m_inventory.SetItem(INVENTORY, inventorySlot, itemData);
    if (!setRes.has_value()) {
        EterBase::ModernLogger::Error("SafeboxCommandHandler::MoveItemToInventory: Failed to set item in inventory slot {}", inventorySlot.get());
        return std::unexpected(CommandError::InvalidSlot);
    }

    auto removeRes = m_safebox.RemoveItem(safeboxSlot);
    if (!removeRes.has_value()) {
        EterBase::ModernLogger::Error("SafeboxCommandHandler::MoveItemToInventory: Failed to remove item from safebox slot {}", safeboxSlot.get());
        // Attempt rollback
        (void)m_inventory.RemoveItem(INVENTORY, inventorySlot);
        return std::unexpected(CommandError::InvalidSlot);
    }

    EterBase::ModernLogger::Info("SafeboxCommandHandler::MoveItemToInventory: Item moved from safebox {} to inventory {}", safeboxSlot.get(), inventorySlot.get());
    return {};
}

EterBase::Result<void, CommandError> SafeboxCommandHandler::Close() {
    if (!m_isOpen) {
        EterBase::ModernLogger::Error("SafeboxCommandHandler::Close: SafeBox is already closed");
        return std::unexpected(CommandError::NotOpened);
    }

    m_isOpen = false;
    EterBase::ModernLogger::Info("SafeboxCommandHandler::Close: SafeBox closed successfully");
    return {};
}

bool SafeboxCommandHandler::IsOpen() const {
    return m_isOpen;
}

} // namespace Client::Gameplay
