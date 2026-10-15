#pragma once

#include <cstdint>
#include <unordered_map>
#include <optional>
#include <functional>
#include <vector>
#include <string>

namespace World {

enum class MotionMode : uint32_t {
    General = 0,
    Horse = 1,
    Bow = 2,
    Sword = 3,
    TwoHanded = 4,
    Dagger = 5,
    Bell = 6,
    Fan = 7
};

enum class MotionState : uint32_t {
    Wait = 0,
    Walk = 1,
    Run = 2,
    Attack = 3,
    Skill = 4,
    Dead = 5,
    MountWait = 6,
    MountRun = 7,
    Stun = 8,
    Knockback = 9,
    Damage = 10,
    Fishing = 11,
    Mining = 12,
    Count
};

struct MotionKey {
    MotionMode mode;
    MotionState state;

    bool operator==(const MotionKey& other) const {
        return mode == other.mode && state == other.state;
    }
};

} // namespace World

template <>
struct std::hash<World::MotionKey> {
    std::size_t operator()(const World::MotionKey& k) const noexcept {
        return (static_cast<std::size_t>(k.mode) << 16) ^ static_cast<std::size_t>(k.state);
    }
};

namespace World {

class ActorMotionMachine {
public:
    ActorMotionMachine();
    ~ActorMotionMachine() = default;

    void RegisterMotion(MotionMode mode, MotionState state, uint32_t motionId);
    std::optional<uint32_t> GetMotionId(MotionMode mode, MotionState state) const;

    // State machine management
    bool ChangeState(MotionState newState);
    bool ChangeMode(MotionMode newMode);
    
    MotionState GetCurrentState() const { return m_currentState; }
    MotionMode GetCurrentMode() const { return m_currentMode; }

    std::optional<uint32_t> GetCurrentMotionId() const;
    
    // Updates internal timing (returns true if motion finished)
    bool Update(float deltaTime);

    // Forces a state change bypassing transition rules
    void ForceState(MotionState newState);

    bool IsTransitionAllowed(MotionState from, MotionState to) const;
    void SetMotionDuration(uint32_t motionId, float duration);
    float GetMotionProgress() const;

private:
    void SetupTransitions();

    std::unordered_map<MotionKey, uint32_t> m_motions;
    std::unordered_map<uint32_t, float> m_motionDurations;
    
    MotionMode m_currentMode;
    MotionState m_currentState;

    float m_currentTimeInState;
    float m_currentStateDuration;

    // Transition matrix
    std::vector<std::vector<bool>> m_validTransitions;
};

} // namespace World
