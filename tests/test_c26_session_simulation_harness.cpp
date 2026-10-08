#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/Client/Simulation/SessionSimulationHarness.h"
#include "../src/Client/Core/WorldContext.h"
#include <cmath>

TEST_CASE("SessionSimulationHarness - Basic Simulation flow") {
    Client::Simulation::SessionSimulationHarness harness;
    
    SUBCASE("AdvanceTime and StepFrames do not throw") {
        CHECK_NOTHROW(harness.AdvanceTime(0.1f));
        CHECK_NOTHROW(harness.StepFrames(10));
    }
    
    SUBCASE("Spawn methods execute without error") {
        CHECK_NOTHROW(harness.SpawnPlayer(1001, "TestHero", 500.0f, 600.0f));
        CHECK_NOTHROW(harness.SpawnMonster(2001, 101, 550.0f, 650.0f, 1500));
        CHECK_NOTHROW(harness.DropItem(3001, 2799, 520.0f, 620.0f));
    }

    SUBCASE("Assertion methods validate entity state") {
        CHECK_NOTHROW(harness.AssertEntityExists(1001));
        CHECK_NOTHROW(harness.AssertEntityDead(2001));
        
        // Mock set local player coords for assertion check
        auto& ctx = harness.GetSession().GetWorldContext();
        ctx.localPlayerCoords.x = 100.0f;
        ctx.localPlayerCoords.y = 200.0f;
        
        CHECK_NOTHROW(harness.AssertPlayerCoords(100.0f, 200.0f));
        CHECK_THROWS(harness.AssertPlayerCoords(0.0f, 0.0f));
    }
}
