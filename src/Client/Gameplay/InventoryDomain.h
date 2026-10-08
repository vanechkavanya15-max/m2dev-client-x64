#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <expected>
#include <span>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../../UserInterface/Core/EventBus.h"

#include "../../UserInterface/GameType.h"

namespace Client::Gameplay {

/**
 * @brief Represents the dimensions of an item in the inventory grid.
 */
struct ItemSize {
    uint8_t width;
    uint8_t height;
};

/**
 * @brief Core data structure representing an item in the combined inventory domain.
 */
struct ItemData {
    EterBase::ItemVnum vnum;
    uint32_t count;
    ItemSize size; // For anti-overflow validation
};

/**
 * @brief Event triggered when an inventory slot changes.
 */
struct InventorySlotUpdatedEvent : public UserInterface::Core::IEvent {
    uint8_t windowType;
    EterBase::ItemSlot slotIndex;

    explicit InventorySlotUpdatedEvent(uint8_t windowType, EterBase::ItemSlot slotIndex)
        : windowType(windowType), slotIndex(slotIndex) {}
};

/**
 * @brief Centralized Domain Model for managing the player's complete inventory state.
 * 
 * Handles Main Inventory (pages 1-4), Belt, Equipment, Dragon Soul, and SafeBox.
 * Provides validation for item size (1x1, 1x2, 1x3) and prevents overlapping.
 */
class InventoryDomain {
public:
    // Standard Metin2 inventory dimensions
    static constexpr uint16_t INVENTORY_PAGE_WIDTH = 5;
    static constexpr uint16_t INVENTORY_PAGE_HEIGHT = 9;
    static constexpr uint16_t INVENTORY_PAGE_SIZE = INVENTORY_PAGE_WIDTH * INVENTORY_PAGE_HEIGHT;
    static constexpr uint16_t INVENTORY_MAX_PAGES = 4;
    static constexpr uint16_t INVENTORY_MAX_NUM = INVENTORY_PAGE_SIZE * INVENTORY_MAX_PAGES;
    
    // Other window sizes (example sizes, adjust according to actual GameType constants if needed)
    static constexpr uint16_t BELT_INVENTORY_MAX_NUM = 16;
    static constexpr uint16_t EQUIPMENT_MAX_NUM = 32;
    static constexpr uint16_t DRAGON_SOUL_INVENTORY_MAX_NUM = 32 * 6 * 2; // Example
    static constexpr uint16_t SAFEBOX_MAX_NUM = 135;

    InventoryDomain();
    ~InventoryDomain() = default;

    /**
     * @brief Sets an item at the specified slot, validating overlap and bounds.
     */
    std::expected<void, EterBase::InventoryError> SetItem(uint8_t windowType, EterBase::ItemSlot slot, const ItemData& item);

    /**
     * @brief Removes an item from the specified slot.
     */
    std::expected<void, EterBase::InventoryError> RemoveItem(uint8_t windowType, EterBase::ItemSlot slot);

    /**
     * @brief Swaps the items between two slots, validating target slots and sizes.
     */
    std::expected<void, EterBase::InventoryError> SwapItem(uint8_t windowType, EterBase::ItemSlot srcSlot, uint8_t dstWindowType, EterBase::ItemSlot dstSlot);

    /**
     * @brief Splits an item stack, placing the split amount into an empty target slot.
     */
    std::expected<void, EterBase::InventoryError> SplitItem(uint8_t windowType, EterBase::ItemSlot srcSlot, EterBase::ItemSlot dstSlot, uint32_t splitCount);

    /**
     * @brief Retrieves the item at the specified slot.
     */
    std::expected<ItemData, EterBase::InventoryError> GetItem(uint8_t windowType, EterBase::ItemSlot slot) const;

private:
    std::expected<std::reference_wrapper<std::vector<std::optional<ItemData>>>, EterBase::InventoryError> GetWindowSlots(uint8_t windowType);
    std::expected<std::reference_wrapper<const std::vector<std::optional<ItemData>>>, EterBase::InventoryError> GetWindowSlots(uint8_t windowType) const;

    bool IsValidCell(uint8_t windowType, EterBase::ItemSlot slot, ItemSize size) const;
    bool IsEmpty(uint8_t windowType, EterBase::ItemSlot slot, ItemSize size, std::optional<EterBase::ItemSlot> ignoreSlot = std::nullopt) const;

    std::vector<std::optional<ItemData>> m_mainInventory;
    std::vector<std::optional<ItemData>> m_beltInventory;
    std::vector<std::optional<ItemData>> m_equipment;
    std::vector<std::optional<ItemData>> m_dragonSoulInventory;
    std::vector<std::optional<ItemData>> m_safeBox;
};

} // namespace Client::Gameplay
