#include "ItemBonusCalculator.h"

namespace Client::Gameplay {

int32_t ItemBonusCalculator::CalculateTotalBonus(uint8_t bonusType, const std::vector<EquippedItemInfo>& equippedItems) const {
    if (bonusType == 0) {
        return 0;
    }
    int32_t total = 0;
    for (const auto& item : equippedItems) {
        for (const auto& attr : item.attributes) {
            if (attr.type == bonusType) {
                total += attr.value;
            }
        }
    }
    return total;
}

std::unordered_map<uint8_t, int32_t> ItemBonusCalculator::AggregateAllBonuses(const std::vector<EquippedItemInfo>& equippedItems) const {
    std::unordered_map<uint8_t, int32_t> resultMap;
    for (const auto& item : equippedItems) {
        for (const auto& attr : item.attributes) {
            if (attr.type != 0 && attr.value != 0) {
                resultMap[attr.type] += attr.value;
            }
        }
    }
    return resultMap;
}

} // namespace Client::Gameplay
