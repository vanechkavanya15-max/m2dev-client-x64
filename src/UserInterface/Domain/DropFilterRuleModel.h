#pragma once

#include <cstdint>
#include <optional>
#include <expected>
#include <string_view>
#include <format>
#include <vector>
#include <algorithm>

#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "UserInterface/Core/EventBus.h"

/**
 * @file DropFilterRuleModel.h
 * @brief Domain model for managing and applying drop filter rules in the Metin2 client.
 */

namespace Domain {

/**
 * @brief Defines the categories of items that can be filtered.
 */
enum class DropFilterCategory : uint8_t {
    White = 0,    ///< Normal, common items (e.g., white text)
    Yellow,       ///< Valuable, rare items (e.g., yellow text)
    Ignored,      ///< Items explicitly ignored by the player
    Equipment,    ///< Weapons and armors
    Consumable,   ///< Potions, food, etc.
    Unknown
};

/**
 * @brief Event triggered when drop filter settings are changed.
 */
struct DropFilterChangedEvent : public UserInterface::Core::IEvent {
    DropFilterCategory category;
    bool isEnabled;

    /**
     * @brief Constructs a new DropFilterChangedEvent.
     * @param category The category that was modified.
     * @param isEnabled The new state of the filter for the category.
     */
    DropFilterChangedEvent(DropFilterCategory category, bool isEnabled)
        : category(category), isEnabled(isEnabled) {}
};

/**
 * @brief Domain model class for managing drop filter rules.
 * 
 * This class adheres to C++23 standards, avoiding Hungarian notation and using
 * modern C++ features such as std::expected, std::optional, and std::format.
 */
class DropFilterRuleModel {
public:
    /**
     * @brief Constructs the DropFilterRuleModel with default settings.
     */
    DropFilterRuleModel() = default;

    /**
     * @brief Enables or disables the pickup filter for a specific category.
     * 
     * @param category The category to modify.
     * @param enable True to enable picking up items of this category, false to ignore them.
     * @return EterBase::PacketResult<void> Returns success or a packet error on failure.
     */
    EterBase::PacketResult<void> SetCategoryFilter(DropFilterCategory category, bool enable) {
        if (category == DropFilterCategory::Unknown) {
            EterBase::ModernLogger::Error("Attempted to set filter for Unknown category");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        bool stateChanged = false;
        
        auto it = std::find_if(filters.begin(), filters.end(),
            [category](const auto& pair) { return pair.first == category; });
            
        if (it != filters.end()) {
            if (it->second != enable) {
                it->second = enable;
                stateChanged = true;
            }
        } else {
            filters.emplace_back(category, enable);
            stateChanged = true;
        }

        if (stateChanged) {
            EterBase::ModernLogger::Info("Drop filter for category {} set to {}", 
                static_cast<uint8_t>(category), enable);
            
            // Publish the event to decouple from GUI
            UserInterface::Core::EventBus::GetInstance().Publish(DropFilterChangedEvent(category, enable));
        }

        return {};
    }

    /**
     * @brief Checks if a specific category is allowed by the current rules.
     * 
     * @param category The category to check.
     * @return std::optional<bool> The filter state if defined, or std::nullopt if not set.
     */
    std::optional<bool> IsCategoryAllowed(DropFilterCategory category) const {
        auto it = std::find_if(filters.begin(), filters.end(),
            [category](const auto& pair) { return pair.first == category; });
            
        if (it != filters.end()) {
            return it->second;
        }
        
        return std::nullopt;
    }

    /**
     * @brief Evaluates whether an item with a given VNUM should be picked up.
     * 
     * @param vnum The strong type representing the item VNUM.
     * @param category The category of the item.
     * @return std::expected<bool, EterBase::InventoryError> True if it should be picked up, false otherwise.
     */
    std::expected<bool, EterBase::InventoryError> ShouldPickupItem(EterBase::ItemVnum vnum, DropFilterCategory category) const {
        if (!vnum) {
            return std::unexpected(EterBase::InventoryError::InvalidVnum);
        }

        bool allowed = IsCategoryAllowed(category).value_or(true); // Default to picking up if no filter is set

        if (!allowed) {
            EterBase::ModernLogger::Debug("Item {} ignored due to drop filter rule.", vnum.get());
        }

        return allowed;
    }

private:
    std::vector<std::pair<DropFilterCategory, bool>> filters;
};

} // namespace Domain
