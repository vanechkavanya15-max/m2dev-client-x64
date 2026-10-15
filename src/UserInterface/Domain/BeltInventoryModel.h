#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <expected>
#include <string_view>
#include <string>
#include <span>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace UserInterface::Domain {

/**
 * @brief Represents the data of an item specifically placed in the belt inventory.
 */
struct BeltItemData
{
    EterBase::ItemVnum vnum; ///< Virtual number of the item
    uint32_t count;          ///< Quantity of the item
};

/**
 * @brief Event triggered to request a refresh of the belt inventory UI.
 * 
 * Used to decouple the domain model from the GUI layer.
 */
struct BeltInventorySlotUpdateEvent : public Core::IEvent
{
    EterBase::ItemSlot slotIndex;

    /**
     * @brief Constructs the event for a specific slot.
     * @param slotIndex The index of the slot that was updated.
     */
    explicit BeltInventorySlotUpdateEvent(EterBase::ItemSlot slotIndex) : slotIndex(slotIndex) {}
};

/**
 * @brief Manages the belt inventory domain logic and memory state.
 * 
 * Strictly follows C++23 standards, utilizing monadic std::optional,
 * std::expected for robust error handling, and event-driven updates.
 */
class BeltInventoryModel
{
public:
    /**
     * @brief Constructs a BeltInventoryModel with a specific capacity.
     * @param capacity The total number of slots in the belt inventory.
     */
    explicit BeltInventoryModel(EterBase::ItemSlot capacity)
        : slots(capacity.get())
    {
    }

    /**
     * @brief Sets an item at the specified slot index.
     * @param index The slot index.
     * @param item The item data to set.
     * @return Result containing void on success, or an InventoryError on failure.
     */
    std::expected<void, EterBase::InventoryError> setItem(EterBase::ItemSlot index, const BeltItemData& item)
    {
        if (index.get() >= slots.size())
        {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "Attempted to set item out of bounds. Index: {}", index.get());
            return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
        }

        slots[index.get()] = item;
        Core::EventBus::GetInstance().Publish(BeltInventorySlotUpdateEvent(index));
        
        return {};
    }

    /**
     * @brief Clears the item at the specified slot.
     * @param index The slot index.
     * @return Result containing void on success, or an InventoryError on failure.
     */
    std::expected<void, EterBase::InventoryError> clearItem(EterBase::ItemSlot index)
    {
        if (index.get() >= slots.size())
        {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "Attempted to clear item out of bounds. Index: {}", index.get());
            return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
        }

        if (!slots[index.get()].has_value())
        {
            return std::unexpected(EterBase::InventoryError::SlotEmpty);
        }

        slots[index.get()].reset();
        Core::EventBus::GetInstance().Publish(BeltInventorySlotUpdateEvent(index));
        
        return {};
    }

    /**
     * @brief Retrieves the item at the specified slot.
     * @param index The slot index.
     * @return std::expected containing the item data if present and valid index, InventoryError otherwise.
     */
    std::expected<BeltItemData, EterBase::InventoryError> getItem(EterBase::ItemSlot index) const
    {
        if (index.get() >= slots.size())
        {
            return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
        }

        if (slots[index.get()].has_value()) {
            return slots[index.get()].value();
        } else {
            return std::unexpected(EterBase::InventoryError::SlotEmpty);
        }
    }

    /**
     * @brief Returns the total capacity of the belt inventory.
     * @return The number of slots.
     */
    EterBase::ItemSlot getCapacity() const
    {
        return EterBase::ItemSlot(static_cast<uint16_t>(slots.size()));
    }

private:
    std::vector<std::optional<BeltItemData>> slots;  ///< Array of belt item slots
};

} // namespace UserInterface::Domain
