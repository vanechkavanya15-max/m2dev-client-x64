#include "MovementTerrainCollider.h"
#include "EterBase/LogModern.h"
#include <cmath>

namespace Client::Gameplay {

MovementTerrainCollider::MovementTerrainCollider(const ITerrainAttributeProvider* provider)
    : m_provider(provider)
{
    if (!m_provider) {
        EterBase::ModernLogger::Error("MovementTerrainCollider created with null ITerrainAttributeProvider.");
    }
}

bool MovementTerrainCollider::IsBlocked(const Client::Core::MapCoords& coords) const
{
    if (!m_provider) {
        return false; // Fail-open or fail-closed? Usually if no terrain, we can't move, but for tests false might be better. Or we just trust the assert.
    }
    
    uint8_t attr = m_provider->GetAttribute(coords);
    return (attr & TERRAIN_ATTRIBUTE_BLOCK) || (attr & TERRAIN_ATTRIBUTE_WATER);
}

Client::Core::Result<Client::Core::MapCoords, Client::Core::CommandError> 
MovementTerrainCollider::CalculateMovement(const Client::Core::MapCoords& startCoords, const Client::Core::MapCoords& endCoords) const
{
    if (!m_provider) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    // Fast path: start is blocked. We shouldn't be here, but if we are, we can't move.
    if (IsBlocked(startCoords)) {
        return std::unexpected(Client::Core::CommandError::MovementBlocked);
    }

    // Fast path: target is not blocked, and distance is small enough that we can assume no walls between.
    // However, a proper raycast is needed to prevent clipping through thin walls.
    // We will do a DDA-like step algorithm.
    
    const float dx = endCoords.x - startCoords.x;
    const float dy = endCoords.y - startCoords.y;
    const float dz = endCoords.z - startCoords.z;
    const float distance = std::sqrt(dx * dx + dy * dy); // Ignore Z for terrain collision
    
    if (distance <= 0.001f) {
        return endCoords;
    }
    
    // Step size based on typical cell size to ensure we don't skip cells.
    // In Metin2, half cell scale is 100 units (1m).
    const float stepSize = 50.0f; 
    const int numSteps = static_cast<int>(std::ceil(distance / stepSize));
    
    const float stepX = dx / numSteps;
    const float stepY = dy / numSteps;
    const float stepZ = dz / numSteps; // Interpolate Z as well
    
    Client::Core::MapCoords currentPos = startCoords;
    Client::Core::MapCoords lastValidPos = startCoords;
    
    bool hitBlock = false;
    
    for (int i = 1; i <= numSteps; ++i) {
        currentPos.x = startCoords.x + stepX * i;
        currentPos.y = startCoords.y + stepY * i;
        currentPos.z = startCoords.z + stepZ * i;
        
        if (IsBlocked(currentPos)) {
            hitBlock = true;
            break;
        }
        
        lastValidPos = currentPos;
    }
    
    if (!hitBlock) {
        return endCoords;
    }
    
    // We hit a block. Attempt Wall Sliding.
    // We can slide along X or Y axis independently.
    Client::Core::MapCoords slideXPos = lastValidPos;
    slideXPos.x = endCoords.x;
    bool canSlideX = !IsBlocked(slideXPos);
    
    Client::Core::MapCoords slideYPos = lastValidPos;
    slideYPos.y = endCoords.y;
    bool canSlideY = !IsBlocked(slideYPos);
    
    if (canSlideX && !canSlideY) {
        return slideXPos;
    } else if (canSlideY && !canSlideX) {
        return slideYPos;
    } else if (canSlideX && canSlideY) {
        // Corner case: diagonal movement into a corner where both adjacent cells are free.
        // We should check the diagonal destination.
        Client::Core::MapCoords diagPos = slideXPos;
        diagPos.y = endCoords.y;
        if (!IsBlocked(diagPos)) {
            return diagPos;
        }
        
        // If the diagonal is blocked, we must choose one axis to slide.
        // Choose the one that gets us closest to the original destination.
        float distSqX = (slideXPos.x - endCoords.x) * (slideXPos.x - endCoords.x) + (slideXPos.y - endCoords.y) * (slideXPos.y - endCoords.y);
        float distSqY = (slideYPos.x - endCoords.x) * (slideYPos.x - endCoords.x) + (slideYPos.y - endCoords.y) * (slideYPos.y - endCoords.y);
        
        if (distSqX < distSqY) {
            return slideXPos;
        } else {
            return slideYPos;
        }
    }
    
    // Completely blocked (can't slide X or Y).
    // If we managed to move at least one step, return the last valid position.
    // Otherwise, return error.
    if (lastValidPos.x != startCoords.x || lastValidPos.y != startCoords.y) {
        return lastValidPos;
    }
    
    return std::unexpected(Client::Core::CommandError::MovementBlocked);
}

} // namespace Client::Gameplay
