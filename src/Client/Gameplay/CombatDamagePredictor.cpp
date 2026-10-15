#include "CombatDamagePredictor.h"
#include <algorithm>
#include <expected>

namespace Client::Gameplay {

CombatDamagePredictor::CombatDamagePredictor() {
    std::random_device rd;
    m_rng.seed(rd());
}

Client::Core::Result<DamagePredictionResult, Client::Core::CommandError> CombatDamagePredictor::PredictDamage(
    const AttackerStats& attacker, 
    const TargetStats& defender) 
{
    // Input validation
    if (attacker.criticalChance > 100 || attacker.penetrateChance > 100 ||
        defender.dodgeChance > 100 || defender.blockChance > 100) 
    {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    std::uniform_int_distribution<uint32_t> dist(1, 100);

    DamagePredictionResult result = {0, false, false, false, false};

    // Roll chances
    result.isDodge = (dist(m_rng) <= defender.dodgeChance);
    result.isBlock = (dist(m_rng) <= defender.blockChance);
    result.isCritical = (dist(m_rng) <= attacker.criticalChance);
    result.isPierce = (dist(m_rng) <= attacker.penetrateChance);

    // If dodge or block occurs, damage is completely mitigated
    if (result.isDodge || result.isBlock) {
        result.damage = 0;
        return result;
    }

    // Base damage calculation avoiding integer overflow
    uint32_t baseDamage = 0;

    // Pierce ignores defense
    if (result.isPierce) {
        baseDamage = attacker.attack;
    } else {
        baseDamage = (attacker.attack > defender.defense) ? (attacker.attack - defender.defense) : 0;
    }

    result.damage = baseDamage;

    // Critical doubles the damage
    if (result.isCritical) {
        result.damage *= 2; // In real scenarios we should also check overflow for this multiplication
    }

    return result;
}

} // namespace Client::Gameplay
