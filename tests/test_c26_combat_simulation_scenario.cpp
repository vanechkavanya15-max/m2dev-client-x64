#include "../src/Client/Simulation/CombatSimulationScenario.h"
#include <cassert>
#include <iostream>

int main() {
    std::cout << "[TEST] Running CombatSimulationScenario...\n";

    Client::Simulation::SessionSimulationHarness harness;
    Client::Simulation::CombatSimulationScenario scenario(harness);

    auto result = scenario.RunDuel(2001, 101, 150);

    assert(result.targetKilled == true);
    assert(result.sequenceValid == true);
    assert(result.attackHits == 3);
    assert(result.totalDamageDealt == 150);

    std::cout << "[PASS] CombatSimulationScenario executed successfully! Details: " << result.details << "\n";
    return 0;
}
