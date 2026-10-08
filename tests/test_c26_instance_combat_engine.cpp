#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "Client/Gameplay/InstanceCombatEngine.h"
#include <thread>
#include <vector>
#include <atomic>

using namespace Client::Gameplay;

TEST_CASE("DamageFlag bitwise operators and mapping") {
    DamageFlag flags = DamageFlag::Normal;
    CHECK(flags == DamageFlag::Normal);

    flags |= DamageFlag::Critical;
    CHECK((flags & DamageFlag::Critical) == DamageFlag::Critical);
    CHECK((flags & DamageFlag::Poison) != DamageFlag::Poison);

    flags |= DamageFlag::Poison;
    CHECK((flags & DamageFlag::Critical) == DamageFlag::Critical);
    CHECK((flags & DamageFlag::Poison) == DamageFlag::Poison);
    
    DamageFlag combo = DamageFlag::Dodge | DamageFlag::Penetrate;
    CHECK((combo & DamageFlag::Dodge) == DamageFlag::Dodge);
    CHECK((combo & DamageFlag::Penetrate) == DamageFlag::Penetrate);
    CHECK((combo & DamageFlag::Block) != DamageFlag::Block);
}

TEST_CASE("InstanceCombatEngine basic add and pop") {
    InstanceCombatEngine engine;
    CHECK(engine.GetPendingDamageCount() == 0);

    engine.AddDamage(100, DamageFlag::Normal, true, false);
    engine.AddDamage(200, DamageFlag::Critical | DamageFlag::Penetrate, false, true);

    CHECK(engine.GetPendingDamageCount() == 2);

    auto damages = engine.PopAllDamages();
    CHECK(damages.size() == 2);
    CHECK(engine.GetPendingDamageCount() == 0);

    CHECK(damages[0].damage == 100);
    CHECK(damages[0].flags == DamageFlag::Normal);
    CHECK(damages[0].isSelf == true);
    CHECK(damages[0].isTarget == false);
    
    CHECK(damages[1].damage == 200);
    CHECK(damages[1].flags == (DamageFlag::Critical | DamageFlag::Penetrate));
    CHECK(damages[1].isSelf == false);
    CHECK(damages[1].isTarget == true);
    
    // Check timestamps are reasonably populated
    CHECK(damages[0].timestampMs > 0);
    CHECK(damages[1].timestampMs > 0);
    CHECK(damages[0].timestampMs <= damages[1].timestampMs);
}

TEST_CASE("InstanceCombatEngine multithreaded add and pop under load") {
    InstanceCombatEngine engine;
    const int num_threads = 8;
    const int damages_per_thread = 10000;
    std::atomic<int> total_popped{0};
    std::atomic<bool> start_flag{false};

    auto producer = [&]() {
        while (!start_flag.load()) { std::this_thread::yield(); }
        for (int i = 0; i < damages_per_thread; ++i) {
            engine.AddDamage(i, DamageFlag::Normal, true, false);
        }
    };

    auto consumer = [&]() {
        while (!start_flag.load()) { std::this_thread::yield(); }
        int empty_polls = 0;
        while (total_popped.load() < num_threads * damages_per_thread) {
            auto popped = engine.PopAllDamages();
            if (popped.empty()) {
                empty_polls++;
                if (empty_polls > 1000) std::this_thread::yield();
            } else {
                empty_polls = 0;
                total_popped += popped.size();
            }
        }
    };

    std::vector<std::thread> producers;
    for (int i = 0; i < num_threads; ++i) {
        producers.emplace_back(producer);
    }

    std::thread consumer_thread(consumer);

    start_flag.store(true);

    for (auto& t : producers) {
        t.join();
    }

    consumer_thread.join();

    auto final_damages = engine.PopAllDamages();
    total_popped += final_damages.size();
    
    CHECK(total_popped.load() == num_threads * damages_per_thread);
    CHECK(engine.GetPendingDamageCount() == 0);
}

TEST_CASE("InstanceCombatEngine PVP Key Registry multithreaded") {
    InstanceCombatEngine::ClearPVPKeys();

    CHECK(InstanceCombatEngine::HasPVPKey(100, 200) == false);

    InstanceCombatEngine::InsertPVPKey(100, 200);
    CHECK(InstanceCombatEngine::HasPVPKey(100, 200) == true);
    CHECK(InstanceCombatEngine::HasPVPKey(200, 100) == false); // Directed

    InstanceCombatEngine::RemovePVPKey(100, 200);
    CHECK(InstanceCombatEngine::HasPVPKey(100, 200) == false);

    const int num_threads = 8;
    const int keys_per_thread = 1000;
    std::atomic<bool> start_flag{false};

    auto worker = [&](int thread_id) {
        while (!start_flag.load()) { std::this_thread::yield(); }
        for (int i = 0; i < keys_per_thread; ++i) {
            uint32_t src = thread_id * keys_per_thread + i;
            uint32_t dst = src + 1;
            InstanceCombatEngine::InsertPVPKey(src, dst);
            bool has = InstanceCombatEngine::HasPVPKey(src, dst);
            // We just ensure we don't crash and lock correctly
            if (i % 2 == 0) {
                InstanceCombatEngine::RemovePVPKey(src, dst);
            }
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i);
    }

    start_flag.store(true);

    for (auto& t : threads) {
        t.join();
    }

    // Final check, odd indexes should still be there, even ones removed
    for (int t = 0; t < num_threads; ++t) {
        for (int i = 0; i < keys_per_thread; ++i) {
            uint32_t src = t * keys_per_thread + i;
            uint32_t dst = src + 1;
            bool expected = (i % 2 != 0);
            CHECK(InstanceCombatEngine::HasPVPKey(src, dst) == expected);
        }
    }

    InstanceCombatEngine::ClearPVPKeys();
    CHECK(InstanceCombatEngine::HasPVPKey(1, 2) == false);
}
