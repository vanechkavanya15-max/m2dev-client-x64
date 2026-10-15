#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include <string>
#include <span>
#include <map>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace UserInterface::Domain {

/**
 * @brief Enum defining the various slots available in the equipment container.
 */
enum class EquipmentSlotType : uint8_t {
    Weapon = 0,
    Armor,
    Helmet,
    Shield,
    Jewelry
};

/**
 * @brief Data structure representing an item equipped by the character.
 */
struct EquipmentItemData {
    EterBase::ItemVnum vnum;                 ///< The virtual number of the item.
    uint32_t count;                          ///< The quantity of the item (usually 1 for equipment).
    std::vector<uint32_t> sockets;           ///< The sockets (e.g., Metin stones) attached to the item.
    
    struct Attribute {
        uint16_t type;
        int16_t value;
    };
    std::vector<Attribute> attributes;       ///< The attributes (bonuses) on the item.
};

/**
 * @brief Event published when an equipment slot is updated.
 */
struct EquipmentSlotUpdatedEvent : public UserInterface::Core::IEvent {
    EquipmentSlotType slotType;
    std::optional<EquipmentItemData> itemData;

    /**
     * @brief Constructs the event with the given slot and optional item data.
     * @param slotType The equipment slot that was updated.
     * @param itemData The new item data in the slot, or std::nullopt if unequipped.
     */
    EquipmentSlotUpdatedEvent(EquipmentSlotType slotType, std::optional<EquipmentItemData> itemData)
        : slotType(slotType), itemData(std::move(itemData)) {}
};

/**
 * @brief Manages the character's equipped items (weapons, armor, etc.).
 * 
 * Decoupled from the GUI layer; uses EventBus to notify observers of changes.
 * Adheres to C++23 standards, utilizing strong types, monadic optional operations, 
 * and std::expected for robust error handling.
 */
class EquipmentContainerModel {
public:
    EquipmentContainerModel() = default;
    ~EquipmentContainerModel() = default;

    /**
     * @brief Equips an item into the specified slot.
     * 
     * @param slotType The target slot.
     * @param itemData The item to equip.
     * @return std::expected<void, EterBase::EntityError> Success if the item is equipped, otherwise an error.
     */
    std::expected<void, EterBase::EntityError> EquipItem(EquipmentSlotType slotType, const EquipmentItemData& itemData) {
        if (equippedItems_.contains(slotType)) {
            EterBase::ModernLogger::Warn("Slot {} is already occupied.", static_cast<uint8_t>(slotType));
            return std::unexpected(EterBase::EntityError::AlreadyExists);
        }

        equippedItems_[slotType] = itemData;
        
        UserInterface::Core::EventBus::GetInstance().Publish(
            EquipmentSlotUpdatedEvent(slotType, itemData)
        );
        
        return {};
    }

    /**
     * @brief Unequips an item from the specified slot.
     * 
     * @param slotType The target slot to unequip.
     * @return std::expected<void, EterBase::EntityError> Success if the item is unequipped, otherwise an error.
     */
    std::expected<void, EterBase::EntityError> UnequipItem(EquipmentSlotType slotType) {
        if (!equippedItems_.contains(slotType)) {
            EterBase::ModernLogger::Warn("Attempted to unequip an empty slot {}.", static_cast<uint8_t>(slotType));
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        equippedItems_.erase(slotType);

        UserInterface::Core::EventBus::GetInstance().Publish(
            EquipmentSlotUpdatedEvent(slotType, std::nullopt)
        );

        return {};
    }

    /**
     * @brief Retrieves the item currently equipped in the specified slot.
     * 
     * @param slotType The target slot.
     * @return std::expected<EquipmentItemData, EterBase::EntityError> The equipped item data, or an error if empty.
     */
    std::expected<EquipmentItemData, EterBase::EntityError> GetItem(EquipmentSlotType slotType) const {
        if (!equippedItems_.contains(slotType)) {
            return std::unexpected(EterBase::EntityError::NotFound);
        }
        
        return equippedItems_.at(slotType);
    }

    /**
     * @brief Check if a specific slot is occupied.
     * 
     * @param slotType The slot to check.
     * @return bool True if the slot has an item equipped, false otherwise.
     */
    bool HasItem(EquipmentSlotType slotType) const {
        return equippedItems_.contains(slotType);
    }

private:
    std::map<EquipmentSlotType, EquipmentItemData> equippedItems_;
};

} // namespace UserInterface::Domain
