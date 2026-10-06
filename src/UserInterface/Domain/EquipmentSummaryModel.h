#pragma once

#include <cstdint>
#include <unordered_map>
#include <optional>
#include <span>
#include <vector>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "InventoryModel.h"

namespace UserInterface::Domain {

/**
 * @brief Event triggered when the equipment summary is updated.
 * 
 * Allows the UI or other systems to refresh whenever an item is equipped or unequipped,
 * modifying the total bonuses without directly coupling to the GUI.
 */
struct EquipmentSummaryUpdatedEvent : public Core::IEvent {
    /**
     * @brief Constructs the event.
     */
    EquipmentSummaryUpdatedEvent() = default;
};

/**
 * @brief Manages the aggregated summary of item attributes (bonuses) from equipped items.
 * 
 * Complies with C++23 standards, utilizing zero Hungarian notation, std::expected for errors,
 * std::optional for lookups, and decouples UI via EventBus.
 */
class EquipmentSummaryModel {
public:
    /**
     * @brief Constructs an empty EquipmentSummaryModel.
     */
    EquipmentSummaryModel() = default;

    /**
     * @brief Adds the attributes of an equipped item to the total summary.
     * 
     * @param attributes A read-only span of attributes to add.
     * @return EterBase::PacketResult<void> Success or packet error.
     */
    EterBase::PacketResult<void> AddItemAttributes(std::span<const ItemAttribute> attributes) {
        for (const auto& attr : attributes) {
            // Ignore empty attributes (type 0)
            if (attr.type == 0) {
                continue;
            }

            attributesSummary[attr.type] += attr.value;
        }

        PublishUpdateEvent();
        return {};
    }

    /**
     * @brief Removes the attributes of an unequipped item from the total summary.
     * 
     * @param attributes A read-only span of attributes to remove.
     * @return EterBase::PacketResult<void> Success or packet error.
     */
    EterBase::PacketResult<void> RemoveItemAttributes(std::span<const ItemAttribute> attributes) {
        for (const auto& attr : attributes) {
            if (attr.type == 0) {
                continue;
            }

            auto it = attributesSummary.find(attr.type);
            if (it != attributesSummary.end()) {
                it->second -= attr.value;
                
                // Optional: remove the key if the value reaches zero to save memory
                // But typically negative values could exist (e.g. curses), 
                // and 0 might be useful to show. Let's keep it simple.
                if (it->second == 0) {
                    attributesSummary.erase(it);
                }
            }
        }

        PublishUpdateEvent();
        return {};
    }

    /**
     * @brief Retrieves the total value for a specific attribute type.
     * 
     * @param type The ID of the attribute type.
     * @return std::optional<int32_t> The total value if the attribute exists, otherwise std::nullopt.
     */
    [[nodiscard]] std::optional<int32_t> GetAttributeTotal(uint16_t type) const {
        if (auto it = attributesSummary.find(type); it != attributesSummary.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Clears all aggregated attributes.
     */
    void Clear() {
        if (!attributesSummary.empty()) {
            attributesSummary.clear();
            PublishUpdateEvent();
        }
    }

private:
    /**
     * @brief Publishes the EquipmentSummaryUpdatedEvent to the EventBus.
     */
    void PublishUpdateEvent() {
        Core::EventBus::GetInstance().Publish(EquipmentSummaryUpdatedEvent{});
    }

    std::unordered_map<uint16_t, int32_t> attributesSummary; ///< Aggregated total of all equipped bonuses.
};

} // namespace UserInterface::Domain
