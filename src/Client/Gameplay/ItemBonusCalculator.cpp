#include "ItemBonusCalculator.h"

namespace Client::Gameplay {

void ItemBonusCalculator::SetSocketBonusResolver(SocketBonusResolver resolver) {
    m_customResolver = std::move(resolver);
}

void ItemBonusCalculator::RegisterSocketBonus(uint32_t socketVnum, uint8_t type, int16_t value) {
    m_registeredBonuses[socketVnum].push_back(ItemAttribute{type, value});
}

std::vector<ItemAttribute> ItemBonusCalculator::GetSocketBonuses(uint32_t socketVnum) const {
    if (socketVnum == 0) {
        return {};
    }

    auto it = m_registeredBonuses.find(socketVnum);
    if (it != m_registeredBonuses.end()) {
        return it->second;
    }

    if (m_customResolver) {
        auto res = m_customResolver(socketVnum);
        if (!res.empty()) {
            return res;
        }
    }

    // Standardowe kamienie dusz Metin2 (+0 do +4): zakres 28030 do 28443
    // Schemat VNUM: 28[grade][subId]
    if (socketVnum >= 28030 && socketVnum <= 28443) {
        uint32_t grade = (socketVnum - 28000) / 100;
        uint32_t sub = socketVnum % 100;

        if (grade <= 4 && sub >= 30 && sub <= 43) {
            static constexpr int16_t s_stoneValues[14][5] = {
                {1, 2, 3, 5, 8},        // 30: Penetration (16)
                {1, 2, 3, 5, 8},        // 31: Critical (15)
                {5, 8, 12, 17, 25},     // 32: Cast speed (9)
                {5, 8, 12, 17, 25},     // 33: Warrior (59)
                {5, 8, 12, 17, 25},     // 34: Assassin (60)
                {5, 8, 12, 17, 25},     // 35: Sura (61)
                {5, 8, 12, 17, 25},     // 36: Shaman (62)
                {1, 2, 3, 5, 8},        // 37: Monster (63)
                {1, 2, 3, 5, 8},        // 38: Block (27)
                {1, 2, 3, 5, 8},        // 39: Dodge (28)
                {30, 60, 90, 120, 150}, // 40: Max SP (2)
                {50, 100, 150, 200, 300},// 41: Max HP (1)
                {2, 4, 6, 10, 15},      // 42: Def grade (54)
                {5, 10, 15, 20, 30}     // 43: Mov speed (8)
            };

            static constexpr uint8_t s_stoneApplyTypes[14] = {
                16, // 30: APPLY_PENETRATE_PCT
                15, // 31: APPLY_CRITICAL_PCT
                9,  // 32: APPLY_CAST_SPEED
                59, // 33: APPLY_ATT_BONUS_TO_WARRIOR
                60, // 34: APPLY_ATT_BONUS_TO_ASSASSIN
                61, // 35: APPLY_ATT_BONUS_TO_SURA
                62, // 36: APPLY_ATT_BONUS_TO_SHAMAN
                63, // 37: APPLY_ATT_BONUS_TO_MONSTER
                27, // 38: APPLY_BLOCK
                28, // 39: APPLY_DODGE
                2,  // 40: APPLY_MAX_SP
                1,  // 41: APPLY_MAX_HP
                54, // 42: APPLY_DEF_GRADE_BONUS
                8   // 43: APPLY_MOV_SPEED
            };

            uint32_t idx = sub - 30;
            uint8_t applyType = s_stoneApplyTypes[idx];
            int16_t applyVal = s_stoneValues[idx][grade];
            return { ItemAttribute{applyType, applyVal} };
        }
    }

    return {};
}

int32_t ItemBonusCalculator::CalculateTotalBonus(uint8_t bonusType, const std::vector<EquippedItemInfo>& equippedItems) const {
    if (bonusType == 0) {
        return 0;
    }
    int32_t total = 0;
    for (const auto& item : equippedItems) {
        // Bonusy ze stalych atrybutow
        for (const auto& attr : item.attributes) {
            if (attr.type == bonusType) {
                total += attr.value;
            }
        }
        // Bonusy z wlozonych kamieni dusz (sockets)
        for (uint32_t socketVal : item.sockets) {
            if (socketVal == 0) continue;
            auto socketBonuses = GetSocketBonuses(socketVal);
            for (const auto& sAttr : socketBonuses) {
                if (sAttr.type == bonusType) {
                    total += sAttr.value;
                }
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
        for (uint32_t socketVal : item.sockets) {
            if (socketVal == 0) continue;
            auto socketBonuses = GetSocketBonuses(socketVal);
            for (const auto& sAttr : socketBonuses) {
                if (sAttr.type != 0 && sAttr.value != 0) {
                    resultMap[sAttr.type] += sAttr.value;
                }
            }
        }
    }
    return resultMap;
}

} // namespace Client::Gameplay
