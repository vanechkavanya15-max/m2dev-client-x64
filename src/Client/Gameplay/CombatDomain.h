#pragma once

#include <cstdint>
#include <vector>
#include <cmath>
#include <mutex>
#include <queue>
#include <optional>
#include <chrono>

namespace Client::Gameplay {

enum class WeaponType : uint8_t {
    Sword = 0,
    TwoHanded = 1,
    Dagger = 2,
    Bow = 3,
    Bell = 4,
    Fan = 5,
    Unarmed = 6
};

enum class DamageFlag : uint32_t {
    None      = 0,
    Critical  = 1 << 0,
    Penetrate = 1 << 1,
    Miss      = 1 << 2,
    Dodge     = 1 << 3,
    Block     = 1 << 4,
    Poison    = 1 << 5,
    Fire      = 1 << 6,
    Bleed     = 1 << 7
};

constexpr DamageFlag operator|(DamageFlag a, DamageFlag b) {
    return static_cast<DamageFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr DamageFlag operator&(DamageFlag a, DamageFlag b) {
    return static_cast<DamageFlag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

constexpr DamageFlag& operator|=(DamageFlag& a, DamageFlag b) {
    a = a | b;
    return a;
}

constexpr bool HasFlag(DamageFlag flags, DamageFlag flag) {
    return (flags & flag) != DamageFlag::None;
}

struct Position3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    float DistanceTo(const Position3D& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        float dz = z - other.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
};

struct Vector2D {
    float x = 0.0f;
    float y = 0.0f;
};

struct Box3D {
    Position3D minExtents;
    Position3D maxExtents;
};

struct CombatDamageEvent {
    uint32_t attackerId;
    uint32_t targetId;
    int32_t damage;
    DamageFlag flags;
    std::chrono::steady_clock::time_point displayTime;
};

class AttackGeometry {
public:
    static float GetWeaponAttackRange(WeaponType weapon);
    static float GetWeaponAttackArc(WeaponType weapon);
    static bool IsInAttackArc(const Position3D& attackerPos, float attackerRotY, const Position3D& targetPos, float arcDegrees);
    static bool CheckTargetHit(const Position3D& attackerPos, float attackerRotY, const Position3D& targetPos, const Box3D& targetBox, WeaponType weapon);
};

class DamagePresentationQueue {
public:
    void EnqueueDamage(const CombatDamageEvent& event);
    std::vector<CombatDamageEvent> PopPresentableDamages();

private:
    std::mutex m_mutex;
    std::queue<CombatDamageEvent> m_queue;
};

class CombatCalculator {
public:
    struct AttackerStats {
        uint8_t level;
        int32_t minDamage;
        int32_t maxDamage;
        int32_t hitChance;
        int32_t criticalChance;
        int32_t penetrateChance;
    };

    struct TargetStats {
        uint8_t level;
        int32_t defense;
        int32_t dodgeChance;
        int32_t blockChance;
    };

    static std::pair<int32_t, DamageFlag> CalculateMeleeStrike(const AttackerStats& attacker, const TargetStats& target, WeaponType weapon);
    static std::pair<int32_t, DamageFlag> CalculateSkillStrike(const AttackerStats& attacker, const TargetStats& target, int32_t skillBaseDamage, int32_t skillMultiplier);
};

} // namespace Client::Gameplay
