#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

#include "src/UserInterface/InstanceComponents/InstanceCombatComponent.h"

using namespace UserInterface::InstanceComponents;

void TestCombatDamages() {
    InstanceCombatComponent combat;
    assert(combat.GetPendingDamageCount() == 0);

    combat.AddDamage(1500, 1, false, true);
    combat.AddDamage(3200, 2, true, false);
    assert(combat.GetPendingDamageCount() == 2);

    auto damages = combat.PopAllDamages();
    assert(damages.size() == 2);
    assert(combat.GetPendingDamageCount() == 0);

    auto it = damages.begin();
    assert(it->damage == 1500);
    assert(it->flag == 1);
    assert(!it->isSelf);
    assert(it->isTarget);

    ++it;
    assert(it->damage == 3200);
    assert(it->flag == 2);
    assert(it->isSelf);
    assert(!it->isTarget);

    combat.AddDamage(500, 0, false, false);
    combat.ClearDamages();
    assert(combat.GetPendingDamageCount() == 0);

    std::cout << "[PASS] TestCombatDamages\n";
}

void TestCombatStateAndCombo() {
    InstanceCombatComponent combat;
    assert(!combat.IsDead());
    assert(!combat.IsStunned());
    assert(combat.GetComboType() == 0);
    assert(combat.GetAttackSpeed() == 100);

    combat.SetDead(true);
    assert(combat.IsDead());

    combat.SetStun(true);
    assert(combat.IsStunned());

    combat.SetComboType(3);
    assert(combat.GetComboType() == 3);

    combat.SetAttackSpeed(150);
    assert(combat.GetAttackSpeed() == 150);

    combat.Clear();
    assert(!combat.IsDead());
    assert(!combat.IsStunned());
    assert(combat.GetComboType() == 0);
    assert(combat.GetAttackSpeed() == 100);

    std::cout << "[PASS] TestCombatStateAndCombo\n";
}

void TestPVPKeyRegistry() {
    InstanceCombatComponent::ClearPVPKeys();

    assert(!InstanceCombatComponent::HasPVPKey(101, 202));
    assert(!InstanceCombatComponent::HasPVPKey(202, 101));

    InstanceCombatComponent::InsertPVPKey(101, 202);
    assert(InstanceCombatComponent::HasPVPKey(101, 202));
    assert(!InstanceCombatComponent::HasPVPKey(202, 101)); // Directed key

    InstanceCombatComponent::RemovePVPKey(101, 202);
    assert(!InstanceCombatComponent::HasPVPKey(101, 202));

    InstanceCombatComponent::InsertPVPKey(10, 20);
    InstanceCombatComponent::InsertPVPKey(30, 40);
    InstanceCombatComponent::ClearPVPKeys();
    assert(!InstanceCombatComponent::HasPVPKey(10, 20));
    assert(!InstanceCombatComponent::HasPVPKey(30, 40));

    std::cout << "[PASS] TestPVPKeyRegistry\n";
}

void TestConcurrentCombatDamages() {
    InstanceCombatComponent combat;
    constexpr int NUM_THREADS = 8;
    constexpr int OPS_PER_THREAD = 500;

    std::vector<std::thread> workers;
    workers.reserve(NUM_THREADS);

    for (int t = 0; t < NUM_THREADS; ++t) {
        workers.emplace_back([&combat, t]() {
            for (int i = 0; i < OPS_PER_THREAD; ++i) {
                combat.AddDamage(static_cast<uint32_t>(t * 1000 + i), 0, false, true);
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    assert(combat.GetPendingDamageCount() == NUM_THREADS * OPS_PER_THREAD);
    auto all = combat.PopAllDamages();
    assert(all.size() == NUM_THREADS * OPS_PER_THREAD);
    assert(combat.GetPendingDamageCount() == 0);

    std::cout << "[PASS] TestConcurrentCombatDamages\n";
}

int main() {
    std::cout << "--- Running test_c26_instance_components ---\n";
    TestCombatDamages();
    TestCombatStateAndCombo();
    TestPVPKeyRegistry();
    TestConcurrentCombatDamages();
    std::cout << "All 4 test cases passed successfully!\n";
    return 0;
}
