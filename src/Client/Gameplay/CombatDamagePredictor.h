#pragma once

#include <cstdint>
#include <random>
#include "../Core/Result.h"
#include "../Core/DomainCommands.h"

namespace Client::Gameplay {

struct AttackerStats {
    uint32_t attack;
    uint32_t criticalChance;
    uint32_t penetrateChance;
};

struct TargetStats {
    uint32_t defense;
    uint32_t dodgeChance;
    uint32_t blockChance;
};

struct DamagePredictionResult {
    uint32_t damage;
    bool isCritical;
    bool isPierce;
    bool isBlock;
    bool isDodge;
};

class CombatDamagePredictor {
public:
    CombatDamagePredictor();

    Client::Core::Result<DamagePredictionResult, Client::Core::CommandError> PredictDamage(
        const AttackerStats& attacker, 
        const TargetStats& defender);

private:
    std::mt19937 m_rng;
};

} // namespace Client::Gameplay
