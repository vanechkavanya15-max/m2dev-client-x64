#pragma once

#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <optional>
#include <array>
#include <algorithm>

namespace metin2::gamelib {

/**
 * @brief Constants related to item data.
 */
constexpr size_t ITEM_NAME_MAX_LEN = 64;
constexpr size_t ITEM_LIMIT_MAX_NUM = 2;
constexpr size_t ITEM_APPLY_MAX_NUM = 3;
constexpr size_t ITEM_VALUES_MAX_NUM = 6;
constexpr size_t ITEM_SOCKET_MAX_NUM = 3;

/**
 * @brief Structure representing a limit on an item.
 */
struct ItemLimit {
    uint8_t type;
    int32_t value;
};

/**
 * @brief Structure representing a static apply (bonus) on an item.
 */
struct ItemApply {
    uint8_t type;
    int32_t value;
};

/**
 * @brief Modern representation of item data, separated from GUI logic.
 */
struct ItemDataEntry {
    uint32_t vnum;
    uint32_t vnumRange;
    std::array<char, ITEM_NAME_MAX_LEN + 1> name;
    std::array<char, ITEM_NAME_MAX_LEN + 1> localeName;

    uint8_t type;
    uint8_t subType;
    uint8_t weight;
    uint8_t size;

    uint32_t antiFlags;
    uint32_t flags;
    uint32_t wearFlags;
    uint32_t immuneFlag;

    uint32_t buyPrice;
    uint32_t sellPrice;

    std::array<ItemLimit, ITEM_LIMIT_MAX_NUM> limits;
    std::array<ItemApply, ITEM_APPLY_MAX_NUM> applies;
    std::array<int32_t, ITEM_VALUES_MAX_NUM> values;
    std::array<int32_t, ITEM_SOCKET_MAX_NUM> sockets;

    uint32_t refinedVnum;
    uint16_t refineSet;
    uint8_t alterToMagicItemPct;
    uint8_t specular;
    uint8_t gainSocketPct;

    /**
     * @brief Get the item name as a string_view.
     * @return std::string_view representing the name.
     */
    [[nodiscard]] std::string_view GetName() const noexcept {
        return std::string_view(name.data());
    }

    /**
     * @brief Get the localized item name as a string_view.
     * @return std::string_view representing the locale name.
     */
    [[nodiscard]] std::string_view GetLocaleName() const noexcept {
        return std::string_view(localeName.data());
    }
};

/**
 * @brief A modern, decoupled registry for fast item data lookups by VNUM.
 */
class ItemDataRegistry {
public:
    ItemDataRegistry() = default;
    ~ItemDataRegistry() = default;

    ItemDataRegistry(const ItemDataRegistry&) = delete;
    ItemDataRegistry& operator=(const ItemDataRegistry&) = delete;

    ItemDataRegistry(ItemDataRegistry&&) noexcept = default;
    ItemDataRegistry& operator=(ItemDataRegistry&&) noexcept = default;

    /**
     * @brief Inserts or updates an item in the registry.
     * @param item The ItemDataEntry to register.
     */
    void RegisterItem(const ItemDataEntry& item) {
        items_[item.vnum] = item;
        if (item.vnumRange > 0) {
            rangeItems_.push_back(item.vnum);
        }
    }

    /**
     * @brief Retrieves a pointer to an item's data by its exact VNUM.
     * @param vnum The unique identifier of the item.
     * @return const ItemDataEntry* Pointer to the item data, or nullptr if not found.
     */
    [[nodiscard]] const ItemDataEntry* GetItemData(uint32_t vnum) const noexcept {
        auto it = items_.find(vnum);
        if (it != items_.end()) {
            return &it->second;
        }

        // Fallback to range search
        for (uint32_t rangeVnum : rangeItems_) {
            auto rangeIt = items_.find(rangeVnum);
            if (rangeIt != items_.end()) {
                const auto& rangeItem = rangeIt->second;
                if (vnum >= rangeItem.vnum && vnum <= rangeItem.vnum + rangeItem.vnumRange) {
                    return &rangeItem;
                }
            }
        }

        return nullptr;
    }

    /**
     * @brief Clears all registered items.
     */
    void Clear() noexcept {
        items_.clear();
        rangeItems_.clear();
    }

    /**
     * @brief Gets the total number of registered unique item definitions.
     * @return size_t number of items.
     */
    [[nodiscard]] size_t Size() const noexcept {
        return items_.size();
    }

private:
    std::unordered_map<uint32_t, ItemDataEntry> items_;
    std::vector<uint32_t> rangeItems_;
};

} // namespace metin2::gamelib
