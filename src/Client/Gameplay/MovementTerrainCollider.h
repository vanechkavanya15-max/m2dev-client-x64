#pragma once

#include <cstdint>
#include <expected>
#include "Client/Core/StrongTypes.h"
#include "Client/Core/DomainCommands.h"
#include "Client/Core/Result.h"

namespace Client::Gameplay {

// Terrain attribute flags from PRTerrainLib/Terrain.h
constexpr uint8_t TERRAIN_ATTRIBUTE_BLOCK = (1 << 0);
constexpr uint8_t TERRAIN_ATTRIBUTE_WATER = (1 << 1);

// Interface for querying terrain attributes
class ITerrainAttributeProvider {
public:
    virtual ~ITerrainAttributeProvider() = default;
    
    // Returns the terrain attribute byte at the given map coordinates (world space / cell scale).
    // The implementation is responsible for converting float coords to grid indices.
    [[nodiscard]] virtual uint8_t GetAttribute(const Client::Core::MapCoords& coords) const = 0;
};

// Collider class that validates movement and calculates wall-sliding
class MovementTerrainCollider {
public:
    MovementTerrainCollider(const ITerrainAttributeProvider* provider);
    ~MovementTerrainCollider() = default;

    // Evaluates a movement from startCoords to endCoords.
    // Performs raycasting to detect walls or water.
    // If a block is hit, it calculates a sliding vector along the wall.
    // Returns the finalized coordinate, or CommandError::MovementBlocked if completely stuck.
    [[nodiscard]] Client::Core::Result<Client::Core::MapCoords, Client::Core::CommandError> 
    CalculateMovement(const Client::Core::MapCoords& startCoords, const Client::Core::MapCoords& endCoords) const;

private:
    const ITerrainAttributeProvider* m_provider;
    
    // Helper to check if a specific coordinate is blocked
    [[nodiscard]] bool IsBlocked(const Client::Core::MapCoords& coords) const;
};

} // namespace Client::Gameplay
