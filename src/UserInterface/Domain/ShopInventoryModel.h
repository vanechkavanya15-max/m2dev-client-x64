#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <span>
#include <string>
#include <expected>
#include <string_view>
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "Core/EventBus.h"

#define Instance GetInstance

namespace UserInterface::Domain {

/**
 * @brief Event triggered when a shop tab or slot is updated.
 */
struct ShopUpdatedEvent : public Core::IEvent {
    uint8_t tabIndex;
    EterBase::ItemSlot slotIndex;

    /**
     * @brief Constructs a shop updated event.
     * @param tabIndex Index of the tab that was updated.
     * @param slotIndex Index of the slot that was updated.
     */
    ShopUpdatedEvent(uint8_t tabIndex, EterBase::ItemSlot slotIndex)
        : tabIndex(tabIndex), slotIndex(slotIndex) {}
};

/**
 * @brief Represents a single attribute/bonus on a shop item.
 */
struct ShopItemAttribute {
    uint16_t type;  ///< Type of the attribute
    int16_t value;  ///< Value of the attribute
};

/**
 * @brief Core data structure representing an item in the shop.
 */
struct ShopItemData {
    EterBase::ItemVnum vnum;                   ///< Virtual number of the item
    uint32_t count;                            ///< Quantity of the item
    uint32_t price;                            ///< Price of the item in the tab's currency
    std::vector<uint32_t> sockets;             ///< Socket values (e.g., Metin stones)
    std::vector<ShopItemAttribute> attributes; ///< Item attributes/bonuses
};

/**
 * @brief Represents a single tab in an NPC shop.
 */
struct ShopTab {
    uint8_t coinType;                                ///< Type of currency (e.g., 0 = Gold, 1 = Secondary Coin)
    std::string name;                                ///< Name of the shop tab
    std::vector<std::optional<ShopItemData>> items;  ///< Array of item slots in this tab
};

/**
 * @brief Manages the shop domain logic and memory state.
 * 
 * Decoupled from the GUI layer; uses UserInterface::Core::EventBus to notify observers of changes.
 */
class ShopInventoryModel {
public:
    /**
     * @brief Constructs a ShopInventoryModel.
     */
    ShopInventoryModel() = default;

    /**
     * @brief Adds a new tab to the shop.
     * @param name The name of the tab.
     * @param coinType The type of currency used in the tab.
     * @param capacity The number of item slots in the tab.
     */
    void addTab(std::string name, uint8_t coinType, size_t capacity) {
        ShopTab tab;
        tab.name = std::move(name);
        tab.coinType = coinType;
        tab.items.resize(capacity);
        EterBase::ModernLogger::Info("ShopInventoryModel: Added tab '{}' with coinType {} and capacity {}", tab.name, coinType, capacity);
        tabs.push_back(std::move(tab));
    }

    /**
     * @brief Sets an item at the specified tab and slot index.
     * @param tabIndex The index of the tab.
     * @param slotIndex The slot index within the tab.
     * @param item The item data to set.
     * @return std::expected containing void on success or string_view error message on failure.
     */
    std::expected<void, std::string_view> setItem(uint8_t tabIndex, EterBase::ItemSlot slotIndex, const ShopItemData& item) {
        auto checkTab = [this, tabIndex]() -> std::expected<std::reference_wrapper<ShopTab>, std::string_view> {
            if (tabIndex >= tabs.size()) return std::unexpected("Tab index out of range");
            return tabs[tabIndex];
        };

        return checkTab().and_then([this, tabIndex, slotIndex, &item](std::reference_wrapper<ShopTab> tabRef) -> std::expected<void, std::string_view> {
            auto& tab = tabRef.get();
            if (slotIndex.get() >= tab.items.size()) return std::unexpected("Slot index out of range");
            tab.items[slotIndex.get()] = item;
            Core::EventBus::Instance().Publish(ShopUpdatedEvent{tabIndex, slotIndex});
            return {};
        });
    }

    /**
     * @brief Clears the item at the specified tab and slot index.
     * @param tabIndex The index of the tab.
     * @param slotIndex The slot index within the tab.
     * @return std::expected containing void on success or string_view error message on failure.
     */
    std::expected<void, std::string_view> clearItem(uint8_t tabIndex, EterBase::ItemSlot slotIndex) {
        auto checkTab = [this, tabIndex]() -> std::expected<std::reference_wrapper<ShopTab>, std::string_view> {
            if (tabIndex >= tabs.size()) return std::unexpected("Tab index out of range");
            return tabs[tabIndex];
        };

        return checkTab().and_then([this, tabIndex, slotIndex](std::reference_wrapper<ShopTab> tabRef) -> std::expected<void, std::string_view> {
            auto& tab = tabRef.get();
            if (slotIndex.get() >= tab.items.size()) return std::unexpected("Slot index out of range");
            tab.items[slotIndex.get()].reset();
            Core::EventBus::Instance().Publish(ShopUpdatedEvent{tabIndex, slotIndex});
            return {};
        });
    }

    /**
     * @brief Retrieves the item at the specified tab and slot index.
     * @param tabIndex The index of the tab.
     * @param slotIndex The slot index within the tab.
     * @return An optional containing the item data if present and indices are valid, otherwise std::nullopt.
     */
    [[nodiscard]] std::optional<ShopItemData> getItem(uint8_t tabIndex, EterBase::ItemSlot slotIndex) const {
        auto getTabOpt = [this, tabIndex]() -> std::optional<std::reference_wrapper<const ShopTab>> {
            if (tabIndex >= tabs.size()) return std::nullopt;
            return tabs[tabIndex];
        };

        return getTabOpt().and_then([slotIndex](std::reference_wrapper<const ShopTab> tabRef) -> std::optional<ShopItemData> {
            const auto& tab = tabRef.get();
            if (slotIndex.get() >= tab.items.size()) return std::nullopt;
            return tab.items[slotIndex.get()];
        });
    }

    /**
     * @brief Gets the number of tabs in the shop.
     * @return The number of tabs.
     */
    [[nodiscard]] size_t getTabCount() const {
        return tabs.size();
    }

    /**
     * @brief Gets the coin type for a specific tab.
     * @param tabIndex The index of the tab.
     * @return An optional containing the coin type if the tab exists.
     */
    [[nodiscard]] std::optional<uint8_t> getTabCoinType(uint8_t tabIndex) const {
        auto getTabOpt = [this, tabIndex]() -> std::optional<std::reference_wrapper<const ShopTab>> {
            if (tabIndex >= tabs.size()) return std::nullopt;
            return tabs[tabIndex];
        };

        return getTabOpt().transform([](std::reference_wrapper<const ShopTab> tabRef) {
            return tabRef.get().coinType;
        });
    }

private:
    std::vector<ShopTab> tabs; ///< List of shop tabs
};

} // namespace UserInterface::Domain

#undef Instance
