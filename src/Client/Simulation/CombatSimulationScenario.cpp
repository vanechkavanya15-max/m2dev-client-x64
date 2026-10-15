#include "CombatSimulationScenario.h"
#include <chrono>

namespace Client::Simulation {

CombatSimulationScenario::CombatSimulationScenario(SessionSimulationHarness& harness)
    : m_harness(harness)
{
}

CombatScenarioResult CombatSimulationScenario::RunDuel(uint32_t monsterVid, uint32_t monsterVnum, int32_t monsterHp) {
    CombatScenarioResult result;
    result.sequenceValid = true;

    // 1. Spawnowanie potwora w swiecie symulacji
    m_harness.SpawnMonster(monsterVid, monsterVnum, 100.0f, 100.0f, static_cast<uint32_t>(monsterHp));

    auto now = std::chrono::steady_clock::now();
    const std::chrono::milliseconds attackInterval(300);

    int32_t currentHp = monsterHp;

    // 2. Deterministyczna petla walki 1v1
    while (currentHp > 0 && result.attackHits < 100) {
        auto seqRes = m_sequenceGuard.ProcessAttackSequence(now, attackInterval);
        if (!seqRes.has_value()) {
            result.sequenceValid = false;
            break;
        }

        int32_t damage = 50;
        currentHp -= damage;
        result.totalDamageDealt += damage;
        result.attackHits++;

        now += attackInterval;
        m_harness.AdvanceTime(0.3f);
    }

    if (currentHp <= 0) {
        result.targetKilled = true;
        result.details = "Monster defeated successfully with valid attack sequences.";
    } else {
        result.details = "Duel ended without kill or rate limited.";
    }

    return result;
}

} // namespace Client::Simulation
