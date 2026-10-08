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

    // Test 5: Dodanie kamieni dusz w gniazda (sockets)
    // 28441: Kamien Witalnosci +4 (Typ 1: Max HP +300) w zbroi
    // 28443: Kamien Przyspieszenia +4 (Typ 8: Szybkosc Ruchu +30) w zbroi
    // 28430: Kamien Przenikania +4 (Typ 16: Cios Przeszywajacy +8%) w mieczu
    armor.sockets[0] = 28441;
    armor.sockets[1] = 28443;
    sword.sockets[0] = 28430;

    std::vector<EquippedItemInfo> eqWithStones = {armor, sword};

    int32_t totalHpWithStones = calc.CalculateTotalBonus(1, eqWithStones);
    assert(totalHpWithStones == 2800); // 2000 + 500 + 300

    int32_t totalMovSpeed = calc.CalculateTotalBonus(8, eqWithStones);
    assert(totalMovSpeed == 30); // 30 z kamienia przyspieszenia

    int32_t totalPenetrate = calc.CalculateTotalBonus(16, eqWithStones);
    assert(totalPenetrate == 8); // 8% z kamienia przenikania

    auto allBonusesWithStones = calc.AggregateAllBonuses(eqWithStones);
    assert(allBonusesWithStones[1] == 2800);
    assert(allBonusesWithStones[23] == 10);
    assert(allBonusesWithStones[5] == 12);
    assert(allBonusesWithStones[8] == 30);
    assert(allBonusesWithStones[16] == 8);
    assert(allBonusesWithStones.size() == 5);

    // Test 6: Rejestracja niestandardowego kamienia dusz (np. VNUM 50000 z wlasnym bonusem)
    calc.RegisterSocketBonus(50000, 6, 25); // Typ 6: Zrecznosc +25
    sword.sockets[1] = 50000;
    std::vector<EquippedItemInfo> eqCustom = {armor, sword};
    int32_t totalDexCustom = calc.CalculateTotalBonus(6, eqCustom);
    assert(totalDexCustom == 25);

    std::cout << "test_c26_item_bonus: ALL TESTS PASSED (100%)\n";
    return 0;
}
