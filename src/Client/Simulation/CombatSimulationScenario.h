#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include "../Core/GameSession.h"
#include "../Gameplay/CombatDomain.h"
#include "../Gameplay/CombatSequenceGuard.h"
#include "SessionSimulationHarness.h"

namespace Client::Simulation {

struct CombatScenarioResult {
    bool targetKilled{false};
    uint32_t attackHits{0};
    int32_t totalDamageDealt{0};
    bool sequenceValid{false};
    std::string details;
};

class CombatSimulationScenario {
public:
    explicit CombatSimulationScenario(SessionSimulationHarness& harness);
    ~CombatSimulationScenario() = default;

    CombatScenarioResult RunDuel(uint32_t monsterVid, uint32_t monsterVnum, int32_t monsterHp);

private:
    SessionSimulationHarness& m_harness;
    Client::Gameplay::CombatSequenceGuard m_sequenceGuard;
};

} // namespace Client::Simulation
