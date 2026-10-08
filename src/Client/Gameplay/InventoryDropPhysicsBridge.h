#pragma once

#include "EterBase/StdAfx.h"



#include "../Core/StrongTypes.h"
#include "../Core/DomainCommands.h"
#include <random>

namespace Client::Gameplay {

/**
 * @brief InventoryDropPhysicsBridge is responsible for calculating the target drop coordinates 
 *        of an item from the player's position when a DropItemCommand is executed.
 *        The physics calculation places the item randomly within a specified radius (up to 200cm).
 */
class InventoryDropPhysicsBridge {
public:
    static constexpr float MAX_DROP_RADIUS = 200.0f;
    static constexpr float MIN_DROP_RADIUS = 50.0f;

    InventoryDropPhysicsBridge();
    ~InventoryDropPhysicsBridge() = default;

    /**
     * @brief Calculates the drop trajectory and final landing coordinates.
     * 
     * @param playerPos The current position of the player.
     * @param command The drop command that triggers this calculation.
     * @return Client::Core::MapCoords The calculated landing coordinates for the item.
     */
    Client::Core::MapCoords CalculateDropTrajectory(const Client::Core::MapCoords& playerPos, const Client::Core::DropItemCommand& command);

private:
    std::mt19937 m_rng;
};

} // namespace Client::Gameplay
