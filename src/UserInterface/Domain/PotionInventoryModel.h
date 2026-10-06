/**
 * @file PotionInventoryModel.h
 * @brief Domain model for managing and detecting potions in the inventory.
 */

#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <expected>
#include <span>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace UserInterface::Domain {

/**
 * @brief Represents the types of potions/elixirs available.
 */
enum class PotionType : uint8_t {
    Red,    ///< HP potion
    Blue,   ///< SP potion
    Green,  ///< Attack speed elixir
    Purple  ///< Movement speed elixir
};

/**
 * @brief Error types for potion inventory operations.
 */
enum class PotionError : uint8_t {
    InvalidSlot,
    SlotEmpty,
    NotAPotion
};

/**
 * @brief Event triggered when a potion slot is updated.
 */
struct PotionSlotUpdateEvent : public UserInterface::Core::IEvent {
    EterBase::ItemSlot slot;
    std::optional<EterBase::ItemVnum> vnum;
    uint32_t count;

    /**
     * @brief Constructs the event.
     * @param slot The slot that was updated.
     * @param vnum The item vnum, if a potion is present.
     * @param count The quantity of the item.
     */
    PotionSlotUpdateEvent(EterBase::ItemSlot slot, std::optional<EterBase::ItemVnum> vnum, uint32_t count)
        : slot(slot), vnum(vnum), count(count) {}
};

/**
 * @brief Represents an item slot holding a potion in the inventory.
 */
struct PotionSlotData {
    EterBase::ItemVnum vnum;
    uint32_t count;
};

/**
 * @brief Manages potion items within the inventory.
 * Decoupled from GUI, uses EventBus for notifications.
 */
class PotionInventoryModel {
public:
    /**
     * @brief Constructs the PotionInventoryModel.
     * @param capacity The number of inventory slots.
     */
    explicit PotionInventoryModel(uint16_t capacity)
        : slots_(capacity) {}

    /**
     * @brief Sets a potion at a specific slot.
     * @param slot The inventory slot.
     * @param vnum The item vnum.
     * @param count The item quantity.
     * @return std::expected<void, PotionError> Success or error code.
     */
    std::expected<void, PotionError> SetPotion(EterBase::ItemSlot slot, EterBase::ItemVnum vnum, uint32_t count) {
        if (slot.get() >= slots_.size()) {
            EterBase::ModernLogger::Warn("Failed to set potion: Invalid slot {}", slot.get());
            return std::unexpected(PotionError::InvalidSlot);
        }

        if (!IsPotion(vnum)) {
            EterBase::ModernLogger::Warn("Failed to set potion: Vnum {} is not a recognized potion", vnum.get());
            return std::unexpected(PotionError::NotAPotion);
        }

        slots_[slot.get()] = PotionSlotData{vnum, count};
        
        // Publish event
        UserInterface::Core::EventBus::GetInstance().Publish(
            PotionSlotUpdateEvent(slot, vnum, count)
        );

        return {};
    }

    /**
     * @brief Clears a potion from a specific slot.
     * @param slot The inventory slot to clear.
     * @return std::expected<void, PotionError> Success or error code.
     */
    std::expected<void, PotionError> ClearSlot(EterBase::ItemSlot slot) {
        if (slot.get() >= slots_.size()) {
            return std::unexpected(PotionError::InvalidSlot);
        }

        if (!slots_[slot.get()].has_value()) {
            return std::unexpected(PotionError::SlotEmpty);
        }

        slots_[slot.get()].reset();
        
        // Publish event
        UserInterface::Core::EventBus::GetInstance().Publish(
            PotionSlotUpdateEvent(slot, std::nullopt, 0)
        );

        return {};
    }

    /**
     * @brief Gets the potion at a specific slot.
     * @param slot The inventory slot.
     * @return std::expected<PotionSlotData, PotionError> The potion data or error code.
     */
    std::expected<PotionSlotData, PotionError> GetPotion(EterBase::ItemSlot slot) const {
        if (slot.get() >= slots_.size()) {
            return std::unexpected(PotionError::InvalidSlot);
        }

        if (!slots_[slot.get()].has_value()) {
            return std::unexpected(PotionError::SlotEmpty);
        }

        return slots_[slot.get()].value();
    }

    /**
     * @brief Finds the first available potion of a given type.
     * @param type The type of potion to find.
     * @return std::optional<EterBase::ItemSlot> The slot containing the potion, if found.
     */
    std::optional<EterBase::ItemSlot> FindFirstPotion(PotionType type) const {
        for (uint16_t i = 0; i < slots_.size(); ++i) {
            std::optional<EterBase::ItemSlot> result = slots_[i]
                .and_then([type](const PotionSlotData& data) -> std::optional<EterBase::ItemVnum> {
                    return IsPotionOfType(data.vnum, type) ? std::optional<EterBase::ItemVnum>{data.vnum} : std::nullopt;
                })
                .transform([i](const EterBase::ItemVnum& /*vnum*/) -> EterBase::ItemSlot {
                    return EterBase::ItemSlot{i};
                });

            if (result.has_value()) {
                return result;
            }
        }
        return std::nullopt;
    }

private:
    std::vector<std::optional<PotionSlotData>> slots_;

    /**
     * @brief Checks if a given vnum is a potion or elixir.
     * @param vnum The item vnum to check.
     * @return true if it's a potion, false otherwise.
     */
    static bool IsPotion(EterBase::ItemVnum vnum) {
        return IsPotionOfType(vnum, PotionType::Red) ||
               IsPotionOfType(vnum, PotionType::Blue) ||
               IsPotionOfType(vnum, PotionType::Green) ||
               IsPotionOfType(vnum, PotionType::Purple);
    }

    /**
     * @brief Checks if a given vnum matches a specific potion type.
     * @param vnum The item vnum to check.
     * @param type The potion type.
     * @return true if the vnum matches the type.
     */
    static bool IsPotionOfType(EterBase::ItemVnum vnum, PotionType type) {
        uint32_t val = vnum.get();
        switch (type) {
            case PotionType::Red:
                return (val >= 27001 && val <= 27003) || 
                       (val >= 27007 && val <= 27009) || 
                       (val == 27051) || (val == 27201) || 
                       (val >= 72723 && val <= 72726);
            case PotionType::Blue:
                return (val >= 27004 && val <= 27006) || 
                       (val == 27052) || (val == 27202) || 
                       (val >= 72727 && val <= 72730);
            case PotionType::Green:
                return (val >= 27100 && val <= 27102) || 
                       (val >= 27110 && val <= 27112);
            case PotionType::Purple:
                return (val >= 27103 && val <= 27105) || 
                       (val >= 27113 && val <= 27115);
            default:
                return false;
        }
    }
};

} // namespace UserInterface::Domain
