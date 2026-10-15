#include <cassert>
#include <iostream>
#include <thread>
#include "Client/Gameplay/SkillCooldownTracker.h"

int main() {
    using namespace Client::Gameplay;
    SkillCooldownTracker tracker;

    // Test 1: Brak aktywnego cooldownu
    assert(!tracker.IsOnCooldown(1, 1000));
    assert(tracker.GetRemainingCooldownMs(1, 1000) == 0);
    assert(tracker.GetCooldownProgress(1, 1000) == 1.0f);

    // Test 2: Rozpoczecie cooldownu na 5000 ms od czasu t=1000
    tracker.StartCooldownWithTimestamp(1, 5000, 1000);
    assert(tracker.IsOnCooldown(1, 1000));
    assert(tracker.IsOnCooldown(1, 3500));
    assert(tracker.GetRemainingCooldownMs(1, 3500) == 2500);
    assert(tracker.GetCooldownProgress(1, 3500) == 0.5f);

    // Test 3: Upłynięcie czasu (t=6000)
    assert(!tracker.IsOnCooldown(1, 6000));
    assert(tracker.GetRemainingCooldownMs(1, 6000) == 0);
    assert(tracker.GetCooldownProgress(1, 6000) == 1.0f);

    // Test 4: Resetowanie cooldownu
    tracker.StartCooldownWithTimestamp(2, 10000, 1000);
    assert(tracker.IsOnCooldown(2, 2000));
    tracker.ResetCooldown(2);
    assert(!tracker.IsOnCooldown(2, 2000));

    std::cout << "test_c26_skill_cooldown: ALL TESTS PASSED (100%)\n";
    return 0;
}
