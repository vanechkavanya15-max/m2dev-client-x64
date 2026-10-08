#include "../src/Client/Gameplay/CombatSequenceGuard.h"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace Client::Gameplay;
using namespace Client::Core;

void Test_CombatSequenceGuard_Success() {
    CombatSequenceGuard guard;
    auto now = std::chrono::steady_clock::now();
    std::chrono::milliseconds interval(300);

    // First attack should succeed
    auto result1 = guard.ProcessAttackSequence(now, interval);
    assert(result1.has_value());
    assert(result1.value() == 1); // 1 % 256 = 1

    // Second attack after interval should succeed
    now += std::chrono::milliseconds(300);
    auto result2 = guard.ProcessAttackSequence(now, interval);
    assert(result2.has_value());
    assert(result2.value() == 2); // 2 % 256 = 2

    std::cout << "Test_CombatSequenceGuard_Success passed!\n";
}

void Test_CombatSequenceGuard_RateLimited() {
    CombatSequenceGuard guard;
    auto now = std::chrono::steady_clock::now();
    std::chrono::milliseconds interval(300);

    // First attack should succeed
    auto result1 = guard.ProcessAttackSequence(now, interval);
    assert(result1.has_value());

    // Second attack too quickly should fail
    now += std::chrono::milliseconds(200); // Less than 300ms
    auto result2 = guard.ProcessAttackSequence(now, interval);
    assert(!result2.has_value());
    assert(result2.error() == CommandError::RateLimited);

    // Third attack after enough time should succeed
    now += std::chrono::milliseconds(100); // 200 + 100 = 300ms from last successful attack
    auto result3 = guard.ProcessAttackSequence(now, interval);
    assert(result3.has_value());

    std::cout << "Test_CombatSequenceGuard_RateLimited passed!\n";
}

int main() {
    Test_CombatSequenceGuard_Success();
    Test_CombatSequenceGuard_RateLimited();
    std::cout << "All CombatSequenceGuard tests passed!\n";
    return 0;
}
