#include <cassert>
#include <iostream>
#include "Client/Gameplay/ItemBonusCalculator.h"

int main() {
    using namespace Client::Gameplay;
    ItemBonusCalculator calc;

    // Przygotowanie wyposazenia: zbroja i miecz
    EquippedItemInfo armor;
    armor.vnum = 11299; // Zbroja z czarnej stali
    armor.attributes[0] = {1, 2000}; // Typ 1: Max HP +2000
    armor.attributes[1] = {23, 10};  // Typ 23: Odpornosc na miecze +10%

    EquippedItemInfo sword;
    sword.vnum = 189; // Miecz zalu
    sword.attributes[0] = {1, 500};  // Typ 1: Max HP +500
    sword.attributes[1] = {5, 12};   // Typ 5: Sila +12

    std::vector<EquippedItemInfo> eq = {armor, sword};

    // Test 1: Suma pojedynczego bonusu (Max HP)
    int32_t totalHp = calc.CalculateTotalBonus(1, eq);
    assert(totalHp == 2500);

    // Test 2: Suma bonusu obecnego tylko w 1 itemie
    int32_t totalStr = calc.CalculateTotalBonus(5, eq);
    assert(totalStr == 12);

    // Test 3: Bonus nieobecny
    int32_t totalDex = calc.CalculateTotalBonus(6, eq);
    assert(totalDex == 0);

    // Test 4: Pelna agregacja do mapy
    auto allBonuses = calc.AggregateAllBonuses(eq);
    assert(allBonuses[1] == 2500);
    assert(allBonuses[23] == 10);
    assert(allBonuses[5] == 12);
    assert(allBonuses.size() == 3);

    std::cout << "test_c26_item_bonus: ALL TESTS PASSED (100%)\n";
    return 0;
}
