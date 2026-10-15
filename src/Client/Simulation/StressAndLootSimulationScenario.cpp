#include "../../EterBase/StdAfx.h"
#include "StressAndLootSimulationScenario.h"
#include <random>

namespace Client::Simulation {

StressAndLootSimulationScenario::StressAndLootSimulationScenario() {
    m_grid = std::make_unique<Client::World::SpatialHashGrid>(1024.0f);
    m_inventory = std::make_unique<Client::Gameplay::InventoryDomain>();
    m_session = std::make_unique<Client::Core::GameSession>();
}

SimulationResult StressAndLootSimulationScenario::RunStressAndLootScenario() {
    SimulationResult result;
    result.success = true;

    auto startTime = std::chrono::high_resolution_clock::now();

    // 1. Spawning 100 monsters
    const uint32_t MONSTER_COUNT = 100;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> posDist(-5000.0f, 5000.0f);

    for (uint32_t i = 1; i <= MONSTER_COUNT; ++i) {
        m_grid->Insert(EterBase::EntityId{i}, posDist(rng), posDist(rng));
        result.monstersSpawned++;
    }

    // 2. Generating 50 dropped items
    const uint32_t DROP_COUNT = 50;
    std::vector<Client::Core::EntityVid> droppedItems;
    for (uint32_t i = MONSTER_COUNT + 1; i <= MONSTER_COUNT + DROP_COUNT; ++i) {
        m_grid->Insert(EterBase::EntityId{i}, posDist(rng), posDist(rng));
        droppedItems.push_back(Client::Core::EntityVid{i});
        result.dropsGenerated++;
    }

    // 3. Simulating PickupCommands and inventory insertion
    // Inventory size is 180 (INVENTORY_MAX_NUM = 180).
    // Start picking up items from slot 150 to verify both successful loot and inventory full errors.
    uint16_t nextSlot = 150; 
    
    for (auto vid : droppedItems) {
        Client::Core::PickupCommand cmd{vid};
        auto cmdResult = m_session->Execute(cmd);
        
        if (cmdResult.has_value()) {
            Client::Gameplay::ItemData item{EterBase::ItemVnum{27001}, 1, {1, 1}}; // 1x1 size item
            
            // Try to place the item
            auto setRes = m_inventory->SetItem(1, EterBase::ItemSlot{nextSlot}, item); // 1 = INVENTORY
            if (setRes.has_value()) {
                result.itemsPickedUp++;
            } else {
                if (setRes.error() == EterBase::InventoryError::SlotOutOfRange || 
                    setRes.error() == EterBase::InventoryError::SlotOccupied) {
                    result.inventoryFullErrors++;
                } else {
                    result.success = false;
                    result.errorMessage = "Unexpected inventory error";
                }
            }
            nextSlot++;
        } else {
            result.success = false;
            result.errorMessage = "Command execution failed";
        }
    }

    // Measure time
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> diff = endTime - startTime;
    result.stepExecutionTimeMs = diff.count();

    // Verify constraints
    if (result.stepExecutionTimeMs > 5.0) {
        result.success = false;
        result.errorMessage = "Execution time exceeded 5ms limit";
    }

    return result;
}

} // namespace Client::Simulation
