#include "CombatDomain.h"
#include <random>
#include <numbers>
#include <algorithm>

namespace Client::Gameplay {

float AttackGeometry::GetWeaponAttackRange(WeaponType weapon) {
    switch (weapon) {
        case WeaponType::Sword:     return 150.0f;
        case WeaponType::TwoHanded: return 180.0f;
        case WeaponType::Dagger:    return 100.0f;
        case WeaponType::Bow:       return 1500.0f;
        case WeaponType::Bell:      return 130.0f;
        case WeaponType::Fan:       return 140.0f;
        case WeaponType::Unarmed:   return 80.0f;
        default:                    return 0.0f;
    }
}

float AttackGeometry::GetWeaponAttackArc(WeaponType weapon) {
    switch (weapon) {
        case WeaponType::Sword:     return 90.0f;
        case WeaponType::TwoHanded: return 120.0f;
        case WeaponType::Dagger:    return 60.0f;
        case WeaponType::Bow:       return 10.0f;
        case WeaponType::Bell:      return 80.0f;
        case WeaponType::Fan:       return 110.0f;
        case WeaponType::Unarmed:   return 45.0f;
        default:                    return 0.0f;
    }
}

bool AttackGeometry::IsInAttackArc(const Position3D& attackerPos, float attackerRotY, const Position3D& targetPos, float arcDegrees) {
    if (arcDegrees >= 360.0f) return true;

    float dx = targetPos.x - attackerPos.x;
    float dy = targetPos.y - attackerPos.y;
    
    if (dx == 0.0f && dy == 0.0f) return true;

    // We adjust std::atan2 so that -Y is 0 degrees (North), +X is 90 degrees (East), etc.
    float angleToTarget = std::atan2(dx, -dy) * 180.0f / std::numbers::pi_v<float>;

    // Normalize angles to [0, 360)
    auto normalizeAngle = [](float a) {
        while (a < 0.0f) a += 360.0f;
        while (a >= 360.0f) a -= 360.0f;
        return a;
    };

    attackerRotY = normalizeAngle(attackerRotY);
    angleToTarget = normalizeAngle(angleToTarget);

    float diff = std::abs(angleToTarget - attackerRotY);
    if (diff > 180.0f) diff = 360.0f - diff;

    return diff <= (arcDegrees / 2.0f);
}

bool AttackGeometry::CheckTargetHit(const Position3D& attackerPos, float attackerRotY, const Position3D& targetPos, const Box3D& targetBox, WeaponType weapon) {
    float range = GetWeaponAttackRange(weapon);
    float arc = GetWeaponAttackArc(weapon);

    // Closest point calculation roughly based on bounding box
    Position3D closestPoint = targetPos;
    closestPoint.x = std::clamp(attackerPos.x, targetPos.x + targetBox.minExtents.x, targetPos.x + targetBox.maxExtents.x);
    closestPoint.y = std::clamp(attackerPos.y, targetPos.y + targetBox.minExtents.y, targetPos.y + targetBox.maxExtents.y);
    closestPoint.z = std::clamp(attackerPos.z, targetPos.z + targetBox.minExtents.z, targetPos.z + targetBox.maxExtents.z);

    if (attackerPos.DistanceTo(closestPoint) > range) {
        return false;
    }

    return IsInAttackArc(attackerPos, attackerRotY, targetPos, arc);
}

void DamagePresentationQueue::EnqueueDamage(const CombatDamageEvent& event) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(event);
}

std::vector<CombatDamageEvent> DamagePresentationQueue::PopPresentableDamages() {
    std::vector<CombatDamageEvent> result;
    auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_queue.empty()) {
        if (m_queue.front().displayTime <= now) {
            result.push_back(m_queue.front());
            m_queue.pop();
        } else {
            break; // Since we push in order, we can assume elements behind are also in the future
        }
    }
    return result;
}

std::pair<int32_t, DamageFlag> CombatCalculator::CalculateMeleeStrike(const AttackerStats& attacker, const TargetStats& target, WeaponType weapon) {
    DamageFlag flags = DamageFlag::None;

    thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<int32_t> chanceDist(1, 100);

    // Calculate level penalty
    int32_t levelDiff = static_cast<int32_t>(target.level) - static_cast<int32_t>(attacker.level);
    int32_t hitPenalty = 0;
    if (levelDiff > 0) {
        hitPenalty = std::min(50, levelDiff * 2);
    }

    // Miss calculation
    int32_t finalHitChance = std::clamp(attacker.hitChance - hitPenalty, 5, 100);
    if (chanceDist(gen) > finalHitChance) {
        return {0, DamageFlag::Miss};
    }

    // Dodge calculation
    if (chanceDist(gen) <= target.dodgeChance) {
        return {0, DamageFlag::Dodge};
    }

    // Block calculation
    if (chanceDist(gen) <= target.blockChance) {
        return {0, DamageFlag::Block};
    }

    // Base damage calculation
    std::uniform_int_distribution<int32_t> dmgDist(attacker.minDamage, attacker.maxDamage);
    int32_t baseDamage = dmgDist(gen);

    // Penetration
    bool penetrate = (chanceDist(gen) <= attacker.penetrateChance);
    int32_t effectiveDefense = penetrate ? 0 : target.defense;
    if (penetrate) {
        flags |= DamageFlag::Penetrate;
    }

    // Defense reduction
    int32_t damageAfterDef = std::max(1, baseDamage - effectiveDefense);

    // Critical
    bool critical = (chanceDist(gen) <= attacker.criticalChance);
    if (critical) {
        damageAfterDef *= 2;
        flags |= DamageFlag::Critical;
    }

    return {damageAfterDef, flags};
}

std::pair<int32_t, DamageFlag> CombatCalculator::CalculateSkillStrike(const AttackerStats& attacker, const TargetStats& target, int32_t skillBaseDamage, int32_t skillMultiplier) {
    DamageFlag flags = DamageFlag::None;
    
    thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<int32_t> chanceDist(1, 100);

    // Skills cannot be dodged or blocked in this domain, only missed by hit chance or leveled penalty.
    int32_t levelDiff = static_cast<int32_t>(target.level) - static_cast<int32_t>(attacker.level);
    int32_t hitPenalty = (levelDiff > 0) ? std::min(50, levelDiff * 2) : 0;
    
    int32_t finalHitChance = std::clamp(attacker.hitChance - hitPenalty, 5, 100);
    if (chanceDist(gen) > finalHitChance) {
        return {0, DamageFlag::Miss};
    }

    // Base damage modified by skill properties
    int32_t baseSkillDamage = skillBaseDamage * skillMultiplier;
    
    // Skill Penetration
    bool penetrate = (chanceDist(gen) <= attacker.penetrateChance);
    int32_t effectiveDefense = penetrate ? 0 : target.defense;
    if (penetrate) {
        flags |= DamageFlag::Penetrate;
    }
    
    int32_t damageAfterDef = std::max(1, baseSkillDamage - effectiveDefense);
    
    // Skill Critical
    bool critical = (chanceDist(gen) <= attacker.criticalChance);
    if (critical) {
        damageAfterDef *= 2; // Crits double the damage after defense
        flags |= DamageFlag::Critical;
    }
    
    return {damageAfterDef, flags};
}

} // namespace Client::Gameplay
