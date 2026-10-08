#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <array>

namespace Client::Gameplay {

struct ItemAttribute {
    uint8_t type{0};
    int16_t value{0};
};

struct EquippedItemInfo {
    uint32_t vnum{0};
    std::array<ItemAttribute, 7> attributes{};
    std::array<uint32_t, 4> sockets{};
};

class ItemBonusCalculator {
public:
    int32_t CalculateTotalBonus(uint8_t bonusType, const std::vector<EquippedItemInfo>& equippedItems) const;
    std::unordered_map<uint8_t, int32_t> AggregateAllBonuses(const std::vector<EquippedItemInfo>& equippedItems) const;
};

} // namespace Client::Gameplay
