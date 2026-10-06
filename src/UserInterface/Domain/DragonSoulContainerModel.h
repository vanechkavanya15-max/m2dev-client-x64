#pragma once

#include <cstdint>
#include <array>
#include <optional>
#include <expected>
#include <span>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace Domain {

/**
 * @brief Events related to Dragon Soul changes, to be broadcasted via EventBus.
 */
struct DragonSoulDeckChangedEvent : public UserInterface::Core::IEvent {
    uint8_t activeDeck;

    /**
     * @brief Constructs the event with the new active deck.
     * @param activeDeck The ID of the newly active deck (e.g., 0 for Deck 1, 1 for Deck 2).
     */
    explicit DragonSoulDeckChangedEvent(uint8_t activeDeck) : activeDeck(activeDeck) {}
};

struct DragonSoulEquipChangedEvent : public UserInterface::Core::IEvent {
    EterBase::ItemSlot slot;
    std::optional<EterBase::ItemVnum> vnum;

    /**
     * @brief Constructs the equip changed event.
     * @param slot The slot that changed.
     * @param vnum The newly equipped vnum, or std::nullopt if unequipped.
     */
    DragonSoulEquipChangedEvent(EterBase::ItemSlot slot, std::optional<EterBase::ItemVnum> vnum) 
        : slot(slot), vnum(vnum) {}
};

/**
 * @brief A modern C++23 container model for managing active Dragon Soul alchemy status.
 *
 * This class is decoupled from the GUI and updates the state of Dragon Soul decks
 * and equipped items in memory. It emits events on state change.
 */
class DragonSoulContainerModel {
public:
    static constexpr size_t DRAGON_SOUL_DECK_MAX_NUM = 2;
    static constexpr size_t DRAGON_SOUL_EQUIP_SLOT_MAX = 6;
    static constexpr size_t TOTAL_EQUIP_SLOTS = DRAGON_SOUL_DECK_MAX_NUM * DRAGON_SOUL_EQUIP_SLOT_MAX;

    /**
     * @brief Constructs an empty Dragon Soul container model.
     */
    DragonSoulContainerModel() = default;

    /**
     * @brief Destroys the Dragon Soul container model.
     */
    ~DragonSoulContainerModel() = default;

    /**
     * @brief Clears all equipped Dragon Souls and resets the active deck.
     */
    void Clear() {
        m_isActive = false;
        m_activeDeck = 0;
        for (auto& slot : m_equipSlots) {
            slot = std::nullopt;
        }
    }

    /**
     * @brief Sets whether the Dragon Soul alchemy system is active for the character.
     *
     * @param active true if active, false otherwise.
     */
    void SetActive(bool active) {
        m_isActive = active;
    }

    /**
     * @brief Checks if the Dragon Soul alchemy system is active.
     *
     * @return true if active, false otherwise.
     */
    [[nodiscard]] bool IsActive() const {
        return m_isActive;
    }

    /**
     * @brief Sets the currently active Dragon Soul deck.
     *
     * @param deck The deck index to set as active (0 to DRAGON_SOUL_DECK_MAX_NUM - 1).
     * @return EterBase::Result<void, EterBase::InventoryError> Success or error if the deck index is out of bounds.
     */
    EterBase::Result<void, EterBase::InventoryError> SetActiveDeck(uint8_t deck) {
        if (deck >= DRAGON_SOUL_DECK_MAX_NUM) {
            EterBase::ModernLogger::Error("Failed to set active deck: {} is out of range", deck);
            return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
        }

        m_activeDeck = deck;
        UserInterface::Core::EventBus::GetInstance().Publish(DragonSoulDeckChangedEvent{deck});
        return {};
    }

    /**
     * @brief Gets the index of the currently active Dragon Soul deck.
     *
     * @return The active deck index.
     */
    [[nodiscard]] uint8_t GetActiveDeck() const {
        return m_activeDeck;
    }

    /**
     * @brief Equips a Dragon Soul item to a specific slot.
     *
     * @param slot The slot to equip the item in (relative to the dragon soul equip inventory start).
     * @param vnum The item vnum to equip.
     * @return EterBase::Result<void, EterBase::InventoryError> Success or error if the slot is out of bounds or already occupied.
     */
    EterBase::Result<void, EterBase::InventoryError> EquipItem(EterBase::ItemSlot slot, EterBase::ItemVnum vnum) {
        if (slot.value() >= TOTAL_EQUIP_SLOTS) {
            EterBase::ModernLogger::Error("Failed to equip item: slot {} is out of range", slot.value());
            return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
        }

        if (!vnum.value()) {
            EterBase::ModernLogger::Error("Failed to equip item: invalid vnum {}", vnum.value());
            return std::unexpected(EterBase::InventoryError::InvalidVnum);
        }

        if (m_equipSlots[slot.value()].has_value()) {
            EterBase::ModernLogger::Error("Failed to equip item: slot {} is already occupied", slot.value());
            return std::unexpected(EterBase::InventoryError::SlotOccupied);
        }

        m_equipSlots[slot.value()] = vnum;
        UserInterface::Core::EventBus::GetInstance().Publish(DragonSoulEquipChangedEvent{slot, vnum});
        return {};
    }

    /**
     * @brief Unequips a Dragon Soul item from a specific slot.
     *
     * @param slot The slot to unequip the item from.
     * @return EterBase::Result<void, EterBase::InventoryError> Success or error if the slot is out of bounds or already empty.
     */
    EterBase::Result<void, EterBase::InventoryError> UnequipItem(EterBase::ItemSlot slot) {
        if (slot.value() >= TOTAL_EQUIP_SLOTS) {
            EterBase::ModernLogger::Error("Failed to unequip item: slot {} is out of range", slot.value());
            return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
        }

        if (!m_equipSlots[slot.value()].has_value()) {
            EterBase::ModernLogger::Error("Failed to unequip item: slot {} is already empty", slot.value());
            return std::unexpected(EterBase::InventoryError::SlotEmpty);
        }

        m_equipSlots[slot.value()] = std::nullopt;
        UserInterface::Core::EventBus::GetInstance().Publish(DragonSoulEquipChangedEvent{slot, std::nullopt});
        return {};
    }

    /**
     * @brief Retrieves the currently equipped item at a specific slot.
     *
     * @param slot The slot to check.
     * @return std::optional<EterBase::ItemVnum> containing the vnum if equipped, or std::nullopt if empty.
     */
    [[nodiscard]] std::optional<EterBase::ItemVnum> GetEquippedItem(EterBase::ItemSlot slot) const {
        if (slot.value() >= TOTAL_EQUIP_SLOTS) {
            return std::nullopt;
        }
        return m_equipSlots[slot.value()];
    }

private:
    bool m_isActive{false}; /**< Whether the Dragon Soul alchemy system is actively applied to stats. */
    uint8_t m_activeDeck{0}; /**< The currently active deck (0 or 1). */

    /**< Array storing the equipped Dragon Souls across all decks. */
    std::array<std::optional<EterBase::ItemVnum>, TOTAL_EQUIP_SLOTS> m_equipSlots{};
};

} // namespace Domain
