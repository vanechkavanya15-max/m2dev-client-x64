#pragma once

#include <cstdint>
#include <chrono>
#include <memory>
#include <vector>
#include <string>

#include "../../EterBase/StrongTypes.h"
#include "../World/SpatialHashGrid.h"
#include "../Gameplay/InventoryDomain.h"
#include "../Core/GameSession.h"

namespace Client::Simulation {

struct SimulationResult {
    bool success{false};
    double stepExecutionTimeMs{0.0};
    uint32_t itemsPickedUp{0};
    uint32_t monstersSpawned{0};
    uint32_t dropsGenerated{0};
    uint32_t inventoryFullErrors{0};
    std::string errorMessage;
};

class StressAndLootSimulationScenario {
public:
    StressAndLootSimulationScenario();
    ~StressAndLootSimulationScenario() = default;

    SimulationResult RunStressAndLootScenario();

private:
    std::unique_ptr<Client::World::SpatialHashGrid> m_grid;
    std::unique_ptr<Client::Gameplay::InventoryDomain> m_inventory;
    std::unique_ptr<Client::Core::GameSession> m_session;
};

} // namespace Client::Simulation
