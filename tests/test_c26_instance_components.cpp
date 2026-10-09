#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

#include "src/UserInterface/InstanceComponents/InstanceCombatComponent.h"
#include "src/UserInterface/InstanceComponents/InstanceBattleComponent.h"

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

void TestPopSingleDamage() {
    InstanceCombatComponent combat;
    assert(!combat.HasDamages());

    combat.AddDamage(100, 1, false, false);
    combat.AddDamage(200, 2, true, false);
    assert(combat.HasDamages());
    assert(combat.GetPendingDamageCount() == 2);

    InstanceCombatComponent::DamageRecord rec;
    bool ok1 = combat.PopDamage(rec);
    assert(ok1);
    assert(rec.damage == 100);
    assert(rec.flag == 1);
    assert(!rec.isSelf);
    assert(combat.GetPendingDamageCount() == 1);

    bool ok2 = combat.PopDamage(rec);
    assert(ok2);
    assert(rec.damage == 200);
    assert(rec.flag == 2);
    assert(rec.isSelf);
    assert(combat.GetPendingDamageCount() == 0);
    assert(!combat.HasDamages());

    bool ok3 = combat.PopDamage(rec);
    assert(!ok3);

    std::cout << "[PASS] TestPopSingleDamage\n";
}

void TestBattleStateAndDuel() {
    InstanceBattleComponent battle;
    assert(battle.GetDuelMode() == 0);
    assert(battle.GetSkillTargetVID() == 0);
    assert(battle.GetLastDmgActorVID() == 0);
    assert(battle.GetAdvActorVID() == 0);
    assert(battle.GetLastComboIndex() == 0);

    battle.SetDuelMode(2);
    assert(battle.GetDuelMode() == 2);

    battle.SetSkillTargetVID(12345);
    assert(battle.GetSkillTargetVID() == 12345);

    battle.SetLastDmgActorVID(67890);
    assert(battle.GetLastDmgActorVID() == 67890);

    battle.SetAdvActorVID(11111);
    assert(battle.GetAdvActorVID() == 11111);

    battle.SetLastComboIndex(4);
    assert(battle.GetLastComboIndex() == 4);

    battle.Clear();
    assert(battle.GetDuelMode() == 0);
    assert(battle.GetSkillTargetVID() == 0);
    assert(battle.GetLastDmgActorVID() == 0);
    assert(battle.GetAdvActorVID() == 0);
    assert(battle.GetLastComboIndex() == 0);

    std::cout << "[PASS] TestBattleStateAndDuel\n";
}

void TestSymmetricPVPAndBattleKeys() {
    InstanceCombatComponent::ClearAllBattleKeys();

    assert(!InstanceCombatComponent::HasSymmetricPVPKey(500, 600));
    assert(!InstanceCombatComponent::HasSymmetricPVPKey(600, 500));

    InstanceCombatComponent::InsertSymmetricPVPKey(500, 600);
    assert(InstanceCombatComponent::HasSymmetricPVPKey(500, 600));
    assert(InstanceCombatComponent::HasSymmetricPVPKey(600, 500)); // Symmetric!

    InstanceCombatComponent::RemoveSymmetricPVPKey(600, 500);
    assert(!InstanceCombatComponent::HasSymmetricPVPKey(500, 600));
    assert(!InstanceCombatComponent::HasSymmetricPVPKey(600, 500));

    // PVP Ready key
    assert(!InstanceCombatComponent::HasPVPReadyKey(1, 2));
    InstanceCombatComponent::InsertPVPReadyKey(1, 2);
    assert(InstanceCombatComponent::HasPVPReadyKey(1, 2));
    assert(InstanceCombatComponent::HasPVPReadyKey(2, 1));

    // GVG key
    assert(!InstanceCombatComponent::HasGVGKey(10, 20));
    InstanceCombatComponent::InsertGVGKey(10, 20);
    assert(InstanceCombatComponent::HasGVGKey(10, 20));
    assert(InstanceCombatComponent::HasGVGKey(20, 10));
    InstanceCombatComponent::RemoveGVGKey(20, 10);
    assert(!InstanceCombatComponent::HasGVGKey(10, 20));

    // DUEL key
    assert(!InstanceCombatComponent::HasDUELKey(99, 88));
    InstanceCombatComponent::InsertDUELKey(99, 88);
    assert(InstanceCombatComponent::HasDUELKey(99, 88));
    assert(InstanceCombatComponent::HasDUELKey(88, 99));

    // Clear all
    InstanceCombatComponent::ClearAllBattleKeys();
    assert(!InstanceCombatComponent::HasPVPReadyKey(1, 2));
    assert(!InstanceCombatComponent::HasDUELKey(99, 88));

    std::cout << "[PASS] TestSymmetricPVPAndBattleKeys\n";
}

int main() {
    std::cout << "--- Running test_c26_instance_components ---\n";
    TestCombatDamages();
    TestCombatStateAndCombo();
    TestPVPKeyRegistry();
    TestConcurrentCombatDamages();
    TestPopSingleDamage();
    TestBattleStateAndDuel();
    TestSymmetricPVPAndBattleKeys();
    std::cout << "All 7 test cases passed successfully!\n";
    return 0;
}
