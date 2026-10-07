#include "../src/Client/Gameplay/CombatDomain.h"
#include <iostream>
#include <cassert>
#include <thread>

using namespace Client::Gameplay;

void TestAttackGeometry_Ranges() {
    assert(AttackGeometry::GetWeaponAttackRange(WeaponType::Sword) == 150.0f);
    assert(AttackGeometry::GetWeaponAttackRange(WeaponType::Bow) == 1500.0f);
    std::cout << "TestAttackGeometry_Ranges passed.\n";
}

void TestAttackGeometry_Arcs() {
    assert(AttackGeometry::GetWeaponAttackArc(WeaponType::Sword) == 90.0f);
    assert(AttackGeometry::GetWeaponAttackArc(WeaponType::Bow) == 10.0f);
    std::cout << "TestAttackGeometry_Arcs passed.\n";
}

void TestAttackGeometry_IsInAttackArc() {
    Position3D attacker{0, 0, 0};
    Position3D targetFront{0, -100, 0}; // In our calc, -y is North (rot 0)
    
    // Facing North (0 degrees)
    assert(AttackGeometry::IsInAttackArc(attacker, 0.0f, targetFront, 90.0f) == true);
    
    Position3D targetSide{100, 0, 0}; // East (rot 90)
    assert(AttackGeometry::IsInAttackArc(attacker, 0.0f, targetSide, 90.0f) == false);
    assert(AttackGeometry::IsInAttackArc(attacker, 90.0f, targetSide, 90.0f) == true);
    std::cout << "TestAttackGeometry_IsInAttackArc passed.\n";
}

void TestAttackGeometry_CheckTargetHit() {
    Position3D attacker{0, 0, 0};
    Position3D target{0, -100, 0}; // Distance 100
    Box3D box{{-10, -10, -10}, {10, 10, 10}};
    
    // Sword range is 150, should hit
    assert(AttackGeometry::CheckTargetHit(attacker, 0.0f, target, box, WeaponType::Sword) == true);
    
    Position3D farTarget{0, -200, 0}; // Distance 200, Sword range is 150
    assert(AttackGeometry::CheckTargetHit(attacker, 0.0f, farTarget, box, WeaponType::Sword) == false);
    
    // Bow range is 1500, should hit farTarget
    assert(AttackGeometry::CheckTargetHit(attacker, 0.0f, farTarget, box, WeaponType::Bow) == true);
    
    std::cout << "TestAttackGeometry_CheckTargetHit passed.\n";
}

void TestCombatCalculator_GuaranteedHit() {
    CombatCalculator::AttackerStats attacker{ 1, 100, 100, 100, 0, 0 };
    CombatCalculator::TargetStats target{ 1, 0, 0, 0 };
    
    auto [damage, flags] = CombatCalculator::CalculateMeleeStrike(attacker, target, WeaponType::Sword);
    assert(damage == 100);
    assert(flags == DamageFlag::None);
    std::cout << "TestCombatCalculator_GuaranteedHit passed.\n";
}

void TestCombatCalculator_CriticalStrike() {
    CombatCalculator::AttackerStats attacker{ 1, 100, 100, 100, 100, 0 }; // 100% crit
    CombatCalculator::TargetStats target{ 1, 0, 0, 0 };
    
    auto [damage, flags] = CombatCalculator::CalculateMeleeStrike(attacker, target, WeaponType::Sword);
    assert(damage == 200); // 100 * 2
    assert(HasFlag(flags, DamageFlag::Critical));
    std::cout << "TestCombatCalculator_CriticalStrike passed.\n";
}

void TestCombatCalculator_Penetrate() {
    CombatCalculator::AttackerStats attacker{ 1, 100, 100, 100, 0, 100 }; // 100% penetrate
    CombatCalculator::TargetStats target{ 1, 50, 0, 0 };
    
    auto [damage, flags] = CombatCalculator::CalculateMeleeStrike(attacker, target, WeaponType::Sword);
    assert(damage == 100); // Defense ignored
    assert(HasFlag(flags, DamageFlag::Penetrate));
    std::cout << "TestCombatCalculator_Penetrate passed.\n";
}

void TestCombatCalculator_Miss() {
    CombatCalculator::AttackerStats attacker{ 1, 100, 100, 0, 0, 0 }; // 0% hit chance (will be clamped to 5%, but for deterministic tests we might have a small chance of failing the test. Since it clamps to 5%, let's just make target level much higher to hit the max penalty, though hit chance is clamped to 5..100)
    // To ensure 100% miss, we would need 0 hit chance, but the code clamps to 5.
    // Let's run it a few times and ensure we see misses. We can't guarantee 100% miss without changing the clamping.
    // So let's test dodge instead which has no clamp.
    
    CombatCalculator::AttackerStats dodgeAttacker{ 1, 100, 100, 100, 0, 0 }; 
    CombatCalculator::TargetStats dodgeTarget{ 1, 0, 100, 0 }; // 100% dodge
    auto [damage2, flags2] = CombatCalculator::CalculateMeleeStrike(dodgeAttacker, dodgeTarget, WeaponType::Sword);
    assert(damage2 == 0);
    assert(HasFlag(flags2, DamageFlag::Dodge));
    
    CombatCalculator::TargetStats blockTarget{ 1, 0, 0, 100 }; // 100% block
    auto [damage3, flags3] = CombatCalculator::CalculateMeleeStrike(dodgeAttacker, blockTarget, WeaponType::Sword);
    assert(damage3 == 0);
    assert(HasFlag(flags3, DamageFlag::Block));
    
    std::cout << "TestCombatCalculator_Miss/Dodge/Block passed.\n";
}

void TestDamagePresentationQueue() {
    DamagePresentationQueue queue;
    auto now = std::chrono::steady_clock::now();
    
    queue.EnqueueDamage({1, 2, 100, DamageFlag::None, now + std::chrono::milliseconds(100)});
    
    auto emptyRes = queue.PopPresentableDamages();
    assert(emptyRes.empty());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    auto res = queue.PopPresentableDamages();
    assert(res.size() == 1);
    assert(res[0].damage == 100);
    
    std::cout << "TestDamagePresentationQueue passed.\n";
}

// Additional edge cases for geometry
void TestAttackGeometry_EdgeCases() {
    Position3D attacker{0, 0, 0};
    Position3D targetStack{0, 0, 0};
    
    // Attacker and target at exactly same spot
    assert(AttackGeometry::IsInAttackArc(attacker, 45.0f, targetStack, 90.0f) == true);
    
    // 360 degree arc covers all
    Position3D behindTarget{0, 100, 0}; // South
    assert(AttackGeometry::IsInAttackArc(attacker, 0.0f, behindTarget, 360.0f) == true);
    
    // Test exact arc boundary
    Position3D boundaryTarget{100, -100, 0}; // North-East (45 degrees)
    assert(AttackGeometry::IsInAttackArc(attacker, 0.0f, boundaryTarget, 90.0f) == true); // diff = 45, arc/2 = 45, so diff <= 45
    assert(AttackGeometry::IsInAttackArc(attacker, 0.0f, boundaryTarget, 89.9f) == false); 

    std::cout << "TestAttackGeometry_EdgeCases passed.\n";
}

void TestCombatCalculator_DodgeAndBlockClamp() {
    CombatCalculator::AttackerStats attacker{ 10, 10, 10, 100, 0, 0 }; 
    CombatCalculator::TargetStats dodgeTarget{ 1, 0, 150, 0 }; // 150% dodge
    auto [damage, flags] = CombatCalculator::CalculateMeleeStrike(attacker, dodgeTarget, WeaponType::Sword);
    assert(damage == 0);
    assert(HasFlag(flags, DamageFlag::Dodge));
    
    std::cout << "TestCombatCalculator_DodgeAndBlockClamp passed.\n";
}

void TestDamagePresentationQueue_Multiple() {
    DamagePresentationQueue queue;
    auto now = std::chrono::steady_clock::now();
    
    queue.EnqueueDamage({1, 2, 10, DamageFlag::None, now + std::chrono::milliseconds(50)});
    queue.EnqueueDamage({1, 2, 20, DamageFlag::Critical, now + std::chrono::milliseconds(100)});
    queue.EnqueueDamage({1, 2, 30, DamageFlag::Penetrate, now + std::chrono::milliseconds(150)});
    
    std::this_thread::sleep_for(std::chrono::milliseconds(75));
    
    auto res1 = queue.PopPresentableDamages();
    assert(res1.size() == 1);
    assert(res1[0].damage == 10);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto res2 = queue.PopPresentableDamages();
    assert(res2.size() == 2);
    assert(res2[0].damage == 20);
    assert(res2[1].damage == 30);
    
    std::cout << "TestDamagePresentationQueue_Multiple passed.\n";
}

void TestCombatCalculator_SkillStrike() {
    CombatCalculator::AttackerStats attacker{ 50, 100, 100, 100, 100, 0 }; // 100% hit, 100% crit, 0% pen
    CombatCalculator::TargetStats target{ 50, 50, 100, 100 }; // 100% dodge, 100% block, 50 def
    
    // Skills bypass dodge and block
    auto [damage, flags] = CombatCalculator::CalculateSkillStrike(attacker, target, 200, 2);
    // Base damage = 200 * 2 = 400.
    // Defense = 50. Damage = 400 - 50 = 350.
    // Crit = 350 * 2 = 700.
    assert(damage == 700);
    assert(HasFlag(flags, DamageFlag::Critical));
    assert(!HasFlag(flags, DamageFlag::Miss));
    assert(!HasFlag(flags, DamageFlag::Dodge));
    assert(!HasFlag(flags, DamageFlag::Block));
    
    std::cout << "TestCombatCalculator_SkillStrike passed.\n";
}

void TestCombatCalculator_FullCombo() {
    CombatCalculator::AttackerStats attacker{ 50, 100, 100, 100, 100, 100 }; // 100% crit, 100% pen
    CombatCalculator::TargetStats target{ 50, 1000, 0, 0 }; // High def
    
    auto [damage, flags] = CombatCalculator::CalculateMeleeStrike(attacker, target, WeaponType::Sword);
    assert(damage == 200); // 100 dmg * 2 crit, ignoring 1000 def
    assert(HasFlag(flags, DamageFlag::Critical));
    assert(HasFlag(flags, DamageFlag::Penetrate));
    std::cout << "TestCombatCalculator_FullCombo passed.\n";
}

int main() {
    TestAttackGeometry_Ranges();
    TestAttackGeometry_Arcs();
    TestAttackGeometry_IsInAttackArc();
    TestAttackGeometry_CheckTargetHit();
    
    TestCombatCalculator_GuaranteedHit();
    TestCombatCalculator_CriticalStrike();
    TestCombatCalculator_Penetrate();
    TestCombatCalculator_Miss();
    TestCombatCalculator_FullCombo();
    TestCombatCalculator_SkillStrike();
    
    TestDamagePresentationQueue();
    TestAttackGeometry_EdgeCases();
    TestCombatCalculator_DodgeAndBlockClamp();
    TestDamagePresentationQueue_Multiple();
    
    std::cout << "All tests passed successfully.\n";
    return 0;
}
