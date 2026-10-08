#include "../src/Client/Gameplay/InventoryItemValidator.h"
#include "../src/GameLib/ItemData.h"
#include "../src/UserInterface/GameType.h"
#include <cassert>
#include <iostream>

// Mock implementation of CItemData for testing purposes
class CItemDataMock : public CItemData {
public:
    bool m_antiFlags[32] = {false};
    bool m_wearFlags[32] = {false};
    TItemLimit m_limits[ITEM_LIMIT_MAX_NUM] = {};

    bool IsAntiFlag(DWORD dwFlag) const {
        for (int i = 0; i < 32; ++i) {
            if ((dwFlag & (1 << i)) && m_antiFlags[i]) return true;
        }
        return false;
    }

    bool IsWearableFlag(DWORD dwFlag) const {
        for (int i = 0; i < 32; ++i) {
            if ((dwFlag & (1 << i)) && m_wearFlags[i]) return true;
        }
        return false;
    }

    bool GetLimit(BYTE byIndex, TItemLimit * pItemLimit) const {
        if (byIndex < ITEM_LIMIT_MAX_NUM) {
            *pItemLimit = m_limits[byIndex];
            return true;
        }
        return false;
    }
};

// Satisfy external linkage for mocked methods during isolated test compilation
#ifdef ISOLATED_TESTING
BOOL CItemData::IsAntiFlag(DWORD dwFlag) const {
    return static_cast<const CItemDataMock*>(this)->IsAntiFlag(dwFlag);
}
BOOL CItemData::IsWearableFlag(DWORD dwFlag) const {
    return static_cast<const CItemDataMock*>(this)->IsWearableFlag(dwFlag);
}
BOOL CItemData::GetLimit(BYTE byIndex, TItemLimit * pItemLimit) const {
    return static_cast<const CItemDataMock*>(this)->GetLimit(byIndex, pItemLimit);
}
#endif

void run_tests() {
    CItemDataMock mockItem;
    const uint8_t RACE_WARRIOR = 0;
    const uint8_t RACE_NINJA = 1;
    
    // Test 1: Valid equip (Body armor)
    mockItem.m_wearFlags[0] = true; // WEARABLE_BODY
    mockItem.m_limits[0].bType = CItemData::LIMIT_LEVEL;
    mockItem.m_limits[0].lValue = 10;
    
    auto result1 = Client::Gameplay::InventoryItemValidator::CanEquipItem(
        &mockItem, RACE_WARRIOR, 15, c_Equipment_Body);
    assert(result1.has_value() && result1.value() == true);

    // Test 2: Level limit fail
    auto result2 = Client::Gameplay::InventoryItemValidator::CanEquipItem(
        &mockItem, RACE_WARRIOR, 5, c_Equipment_Body);
    assert(!result2.has_value());

    // Test 3: Anti-Flag restriction (Warrior cannot wear)
    mockItem.m_antiFlags[2] = true; // ITEM_ANTIFLAG_WARRIOR
    auto result3 = Client::Gameplay::InventoryItemValidator::CanEquipItem(
        &mockItem, RACE_WARRIOR, 15, c_Equipment_Body);
    assert(!result3.has_value());

    // Test 4: Ninja can wear, Warrior cannot
    auto result4 = Client::Gameplay::InventoryItemValidator::CanEquipItem(
        &mockItem, RACE_NINJA, 15, c_Equipment_Body);
    assert(result4.has_value() && result4.value() == true);
    
    // Test 5: Invalid target cell range
    auto result5 = Client::Gameplay::InventoryItemValidator::CanEquipItem(
        &mockItem, RACE_WARRIOR, 15, c_Equipment_Start - 1);
    assert(!result5.has_value());
    
    // Test 6: Invalid Wearable Flag (trying to put Body armor in Head slot)
    auto result6 = Client::Gameplay::InventoryItemValidator::CanEquipItem(
        &mockItem, RACE_NINJA, 15, c_Equipment_Head);
    assert(!result6.has_value());
}

int main() {
    // Note: The execution relies on linking with GameLib or utilizing the mocked implementation above.
    // In our isolated validation, we ensure tests are written and ready for CMake execution.
    #ifdef ISOLATED_TESTING
    run_tests();
    #endif
    std::cout << "All tests prepared successfully.\n";
    return 0;
}
