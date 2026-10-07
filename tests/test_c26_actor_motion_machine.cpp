#include <cassert>
#include <iostream>
#include "../src/Client/World/ActorMotionMachine.h"

using namespace World;

int main() {
    ActorMotionMachine machine;

    // Register some general motions
    machine.RegisterMotion(MotionMode::General, MotionState::Wait, 100);
    machine.RegisterMotion(MotionMode::General, MotionState::Walk, 101);
    machine.RegisterMotion(MotionMode::General, MotionState::Dead, 102);

    // Register some horse motions
    machine.RegisterMotion(MotionMode::Horse, MotionState::Wait, 200);
    machine.RegisterMotion(MotionMode::Horse, MotionState::Run, 201);

    // Test exact match (General Mode)
    auto generalWait = machine.GetMotionId(MotionMode::General, MotionState::Wait);
    assert(generalWait.has_value() && generalWait.value() == 100);

    // Test exact match (Horse Mode)
    auto horseWait = machine.GetMotionId(MotionMode::Horse, MotionState::Wait);
    assert(horseWait.has_value() && horseWait.value() == 200);

    // Test deterministic fallback: Walk on Horse does not exist, should fallback to General
    auto horseWalk = machine.GetMotionId(MotionMode::Horse, MotionState::Walk);
    assert(horseWalk.has_value() && horseWalk.value() == 101); // 101 is General Walk

    // Test fallback from another non-general mode (Bow Mode)
    auto bowDead = machine.GetMotionId(MotionMode::Bow, MotionState::Dead);
    assert(bowDead.has_value() && bowDead.value() == 102); // 102 is General Dead

    // Test motion that does not exist in requested mode nor in General mode
    auto horseAttack = machine.GetMotionId(MotionMode::Horse, MotionState::Attack);
    assert(!horseAttack.has_value());

    // --- New Tests for State Machine Logic ---
    
    // Initial state is Wait
    assert(machine.GetCurrentState() == MotionState::Wait);
    assert(machine.GetCurrentMode() == MotionMode::General);

    // Valid transition Wait -> Walk
    bool transitionOk = machine.ChangeState(MotionState::Walk);
    assert(transitionOk);
    assert(machine.GetCurrentState() == MotionState::Walk);

    // Invalid transition Dead -> Run
    machine.ForceState(MotionState::Dead);
    transitionOk = machine.ChangeState(MotionState::Run);
    assert(!transitionOk); // Should fail
    assert(machine.GetCurrentState() == MotionState::Dead);

    // Valid transition Dead -> Wait (Revive)
    transitionOk = machine.ChangeState(MotionState::Wait);
    assert(transitionOk);
    assert(machine.GetCurrentState() == MotionState::Wait);

    // Test update and duration
    machine.SetMotionDuration(100, 2.0f); // Wait motion has 2 seconds
    machine.ForceState(MotionState::Wait);
    
    assert(machine.GetMotionProgress() == 0.0f);
    
    bool finished = machine.Update(1.0f); // Update 1 sec
    assert(!finished);
    assert(machine.GetMotionProgress() == 0.5f);
    
    finished = machine.Update(1.5f); // Update 1.5 sec more (total 2.5)
    assert(finished);
    assert(machine.GetMotionProgress() == 1.0f);

    std::cout << "All ActorMotionMachine tests passed successfully.\n";
    return 0;
}
