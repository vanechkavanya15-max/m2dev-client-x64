#include "../src/EterBase/StdAfx.h"
#define __CSTATEMANAGER_H // Mock D3D/state manager if necessary

#include "../src/Client/Simulation/StressAndLootSimulationScenario.h"
#include <cassert>
#include <iostream>

using namespace Client::Simulation;

void TestStressAndLootScenario() {
    SessionSimulationHarness harness;
    auto result = harness.RunStressAndLootScenario();

    assert(result.success == true);
    assert(result.monstersSpawned == 100);
    assert(result.dropsGenerated == 50);
    assert(result.itemsPickedUp == 30); // 180 - 150 = 30
    assert(result.inventoryFullErrors == 20); // 50 - 30 = 20
    assert(result.stepExecutionTimeMs <= 5.0);
    
    std::cout << "TestStressAndLootScenario passed.\n";
    std::cout << "Monsters spawned: " << result.monstersSpawned << "\n";
    std::cout << "Drops generated: " << result.dropsGenerated << "\n";
    std::cout << "Items picked up: " << result.itemsPickedUp << "\n";
    std::cout << "Inventory full errors: " << result.inventoryFullErrors << "\n";
    std::cout << "Execution time: " << result.stepExecutionTimeMs << "ms\n";
}

int main() {
    std::cout << "Starting StressAndLootScenario tests...\n";
    TestStressAndLootScenario();
    std::cout << "All tests passed successfully!\n";
    return 0;
}
