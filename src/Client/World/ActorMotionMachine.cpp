#include "StdAfx.h"
#include "ActorMotionMachine.h"
#include <algorithm>
#include <iostream>

namespace World {

ActorMotionMachine::ActorMotionMachine() 
    : m_currentMode(MotionMode::General)
    , m_currentState(MotionState::Wait)
    , m_currentTimeInState(0.0f)
    , m_currentStateDuration(0.0f)
{
    SetupTransitions();
}

void ActorMotionMachine::SetupTransitions() {
    // Initialize valid transitions matrix
    size_t numStates = static_cast<size_t>(MotionState::Count);
    m_validTransitions.assign(numStates, std::vector<bool>(numStates, false));

    auto allow = [&](MotionState from, MotionState to) {
        m_validTransitions[static_cast<size_t>(from)][static_cast<size_t>(to)] = true;
    };

    // Any state can transition to Dead or Damage
    for (size_t i = 0; i < numStates; ++i) {
        allow(static_cast<MotionState>(i), MotionState::Dead);
        allow(static_cast<MotionState>(i), MotionState::Damage);
        allow(static_cast<MotionState>(i), MotionState::Stun);
        allow(static_cast<MotionState>(i), MotionState::Knockback);
    }

    // Dead is a terminal state, nothing goes out of Dead except maybe revive (to Wait)
    allow(MotionState::Dead, MotionState::Wait);

    // Basic movement loop
    allow(MotionState::Wait, MotionState::Walk);
    allow(MotionState::Wait, MotionState::Run);
    allow(MotionState::Walk, MotionState::Wait);
    allow(MotionState::Walk, MotionState::Run);
    allow(MotionState::Run, MotionState::Wait);
    allow(MotionState::Run, MotionState::Walk);

    // Combat
    allow(MotionState::Wait, MotionState::Attack);
    allow(MotionState::Walk, MotionState::Attack);
    allow(MotionState::Run, MotionState::Attack);
    allow(MotionState::Attack, MotionState::Wait);
    allow(MotionState::Attack, MotionState::Walk); // Can walk after attack
    allow(MotionState::Attack, MotionState::Attack); // Combo

    allow(MotionState::Wait, MotionState::Skill);
    allow(MotionState::Walk, MotionState::Skill);
    allow(MotionState::Run, MotionState::Skill);
    allow(MotionState::Skill, MotionState::Wait);

    // Mount states
    allow(MotionState::Wait, MotionState::MountWait);
    allow(MotionState::MountWait, MotionState::Wait); // Dismount
    allow(MotionState::MountWait, MotionState::MountRun);
    allow(MotionState::MountRun, MotionState::MountWait);
    allow(MotionState::MountWait, MotionState::Attack);
    allow(MotionState::MountRun, MotionState::Attack);

    // Recovery from damage/stun
    allow(MotionState::Damage, MotionState::Wait);
    allow(MotionState::Stun, MotionState::Wait);
    allow(MotionState::Knockback, MotionState::Wait);

    // Activities
    allow(MotionState::Wait, MotionState::Fishing);
    allow(MotionState::Fishing, MotionState::Wait);
    allow(MotionState::Wait, MotionState::Mining);
    allow(MotionState::Mining, MotionState::Wait);
}

void ActorMotionMachine::RegisterMotion(MotionMode mode, MotionState state, uint32_t motionId) {
    m_motions[{mode, state}] = motionId;
}

std::optional<uint32_t> ActorMotionMachine::GetMotionId(MotionMode mode, MotionState state) const {
    auto it = m_motions.find({mode, state});
    if (it != m_motions.end()) {
        return it->second;
    }

    // Deterministic fallback logic to General mode
    if (mode != MotionMode::General) {
        auto fallbackIt = m_motions.find({MotionMode::General, state});
        if (fallbackIt != m_motions.end()) {
            return fallbackIt->second;
        }
    }

    return std::nullopt;
}

bool ActorMotionMachine::IsTransitionAllowed(MotionState from, MotionState to) const {
    if (from == to) return true; // Self transition is usually allowed (e.g., combo attack)
    size_t fIdx = static_cast<size_t>(from);
    size_t tIdx = static_cast<size_t>(to);
    
    if (fIdx >= m_validTransitions.size() || tIdx >= m_validTransitions.size()) {
        return false;
    }

    return m_validTransitions[fIdx][tIdx];
}

bool ActorMotionMachine::ChangeState(MotionState newState) {
    if (!IsTransitionAllowed(m_currentState, newState)) {
        return false;
    }

    // Perform state change
    m_currentState = newState;
    m_currentTimeInState = 0.0f;
    
    // Lookup duration
    auto motionId = GetCurrentMotionId();
    if (motionId.has_value()) {
        auto durIt = m_motionDurations.find(motionId.value());
        if (durIt != m_motionDurations.end()) {
            m_currentStateDuration = durIt->second;
        } else {
            m_currentStateDuration = 1.0f; // default 1 second if unknown
        }
    } else {
        m_currentStateDuration = 0.0f; // instant if no motion
    }

    return true;
}

void ActorMotionMachine::ForceState(MotionState newState) {
    m_currentState = newState;
    m_currentTimeInState = 0.0f;
    
    auto motionId = GetCurrentMotionId();
    if (motionId.has_value()) {
        auto durIt = m_motionDurations.find(motionId.value());
        if (durIt != m_motionDurations.end()) {
            m_currentStateDuration = durIt->second;
        } else {
            m_currentStateDuration = 1.0f;
        }
    } else {
        m_currentStateDuration = 0.0f;
    }
}

bool ActorMotionMachine::ChangeMode(MotionMode newMode) {
    m_currentMode = newMode;
    // When mode changes, we might want to check if the current state is valid in the new mode.
    // For now, we trust the fallback logic to handle missing motions.
    return true;
}

std::optional<uint32_t> ActorMotionMachine::GetCurrentMotionId() const {
    return GetMotionId(m_currentMode, m_currentState);
}

void ActorMotionMachine::SetMotionDuration(uint32_t motionId, float duration) {
    m_motionDurations[motionId] = std::max(0.001f, duration); // prevent div by zero
}

bool ActorMotionMachine::Update(float deltaTime) {
    m_currentTimeInState += deltaTime;
    
    if (m_currentStateDuration > 0.0f && m_currentTimeInState >= m_currentStateDuration) {
        return true; // motion finished
    }
    
    return false;
}

float ActorMotionMachine::GetMotionProgress() const {
    if (m_currentStateDuration <= 0.0f) {
        return 1.0f;
    }
    return std::clamp(m_currentTimeInState / m_currentStateDuration, 0.0f, 1.0f);
}

} // namespace World
