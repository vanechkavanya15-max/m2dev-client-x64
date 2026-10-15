#include "EterBase/StdAfx.h"


#include "InventoryDropPhysicsBridge.h"
#include <cmath>
#include <numbers>

namespace Client::Gameplay {

InventoryDropPhysicsBridge::InventoryDropPhysicsBridge() {
    std::random_device rd;
    m_rng.seed(rd());
}

Client::Core::MapCoords InventoryDropPhysicsBridge::CalculateDropTrajectory(
    const Client::Core::MapCoords& playerPos, 
    const Client::Core::DropItemCommand& command) 
{
    // C++23: std::uniform_real_distribution
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * std::numbers::pi_v<float>);
    std::uniform_real_distribution<float> radiusDist(MIN_DROP_RADIUS, MAX_DROP_RADIUS);

    float angle = angleDist(m_rng);
    float radius = radiusDist(m_rng);

    // Calculate delta X and delta Y based on random angle and radius
    float dx = radius * std::cos(angle);
    float dy = radius * std::sin(angle);

    // Form the new target coordinates based on player position
    Client::Core::MapCoords targetPos = playerPos;
    targetPos.x += dx;
    targetPos.y += dy;
    
    // Z-coordinate usually stays the same or drops to the ground level, 
    // we'll keep it exactly the same as player's Z for simple physics here, 
    // assuming a flat terrain in this localized radius.
    
    return targetPos;
}

} // namespace Client::Gameplay
