#include "../src/Client/Gameplay/CombatRangeCalculator.h"
#include <iostream>
#include <cassert>

using namespace Client::Gameplay;
using namespace Client::Core;

void test_sword_in_range() {
    MapCoords p1{0, 0, 0};
    MapCoords p2{150, 0, 0};
    auto result = CombatRangeCalculator::IsTargetInRange(p1, p2, CombatRangeCalculator::WeaponType::Sword);
    assert(result.has_value() && result.value() == true);
}

void test_sword_out_of_range() {
    MapCoords p1{0, 0, 0};
    MapCoords p2{250, 0, 0};
    auto result = CombatRangeCalculator::IsTargetInRange(p1, p2, CombatRangeCalculator::WeaponType::Sword);
    assert(!result.has_value() && result.error() == CommandError::OutOfRange);
}

void test_bow_in_range() {
    MapCoords p1{0, 0, 0};
    MapCoords p2{2400, 0, 0};
    auto result = CombatRangeCalculator::IsTargetInRange(p1, p2, CombatRangeCalculator::WeaponType::Bow);
    assert(result.has_value() && result.value() == true);
}

void test_bow_out_of_range() {
    MapCoords p1{0, 0, 0};
    MapCoords p2{2600, 0, 0};
    auto result = CombatRangeCalculator::IsTargetInRange(p1, p2, CombatRangeCalculator::WeaponType::Bow);
    assert(!result.has_value() && result.error() == CommandError::OutOfRange);
}

void test_dagger_in_range() {
    MapCoords p1{0, 0, 0};
    MapCoords p2{100, 0, 0};
    auto result = CombatRangeCalculator::IsTargetInRange(p1, p2, CombatRangeCalculator::WeaponType::Dagger);
    assert(result.has_value() && result.value() == true);
}

void test_dagger_out_of_range() {
    MapCoords p1{0, 0, 0};
    MapCoords p2{160, 0, 0};
    auto result = CombatRangeCalculator::IsTargetInRange(p1, p2, CombatRangeCalculator::WeaponType::Dagger);
    assert(!result.has_value() && result.error() == CommandError::OutOfRange);
}

void test_twohanded_in_range() {
    MapCoords p1{0, 0, 0};
    MapCoords p2{290, 0, 0};
    auto result = CombatRangeCalculator::IsTargetInRange(p1, p2, CombatRangeCalculator::WeaponType::TwoHanded);
    assert(result.has_value() && result.value() == true);
}

void test_twohanded_out_of_range() {
    MapCoords p1{0, 0, 0};
    MapCoords p2{310, 0, 0};
    auto result = CombatRangeCalculator::IsTargetInRange(p1, p2, CombatRangeCalculator::WeaponType::TwoHanded);
    assert(!result.has_value() && result.error() == CommandError::OutOfRange);
}

int main() {
    test_sword_in_range();
    test_sword_out_of_range();
    test_bow_in_range();
    test_bow_out_of_range();
    test_dagger_in_range();
    test_dagger_out_of_range();
    test_twohanded_in_range();
    test_twohanded_out_of_range();
    std::cout << "All tests passed.\n";
    return 0;
}
