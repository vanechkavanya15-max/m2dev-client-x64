#pragma once

#include <expected>
#include <optional>
#include <format>
#include <cmath>
#include <algorithm>
#include <functional>

#include <d3dx9.h>

#include "../EterBase/Result.h"
#include "../EterBase/StrongTypes.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"
#include "../PRTerrainLib/Terrain.h"
#include "MapManager.h"

namespace GameLib {

/**
 * @brief Event emitted when a raymarch hits the terrain heightmap.
 * Inherits from UserInterface::Core::IEvent for EventBus compatibility.
 */
struct RaymarchHitEvent : public UserInterface::Core::IEvent {
    D3DXVECTOR3 hitPosition;

    /**
     * @brief Constructs a new Raymarch Hit Event object.
     * @param position The exact position of the hit.
     */
    explicit RaymarchHitEvent(const D3DXVECTOR3& position) : hitPosition(position) {}
};

/**
 * @brief Zero-conflict C++23 raymarch terrain query implementation.
 * 
 * Provides static methods to perform fast line-of-sight checks against the terrain heightmap.
 * Uses std::expected for robust error handling without out-parameters.
 */
class RaymarchTerrainQuery {
public:
    /**
     * @brief Performs a fast raymarch through the heightmap to find line-of-sight intersections.
     * 
     * @param mapMgr Reference to the MapManager to access terrain height.
     * @param origin Starting point of the ray in world space.
     * @param direction Normalized direction vector of the ray.
     * @param maxDistance Maximum distance to traverse.
     * @return std::expected<D3DXVECTOR3, EterBase::NavigationError> The intersection point or an error code.
     */
    [[nodiscard]] static std::expected<D3DXVECTOR3, EterBase::NavigationError> ComputeLineOfSight(
        CMapManager& mapMgr, 
        const D3DXVECTOR3& origin, 
        const D3DXVECTOR3& direction, 
        float maxDistance) 
    {
        if (maxDistance <= 0.0f) {
            EterBase::ModernLogger::Warn("ComputeLineOfSight called with invalid maxDistance: {}", maxDistance);
            return std::unexpected(EterBase::NavigationError::DestinationUnreachable);
        }

        // Use monadic optional to safely retrieve outdoor map reference without nested if-statements
        auto getOutdoorMap = [&mapMgr]() -> std::optional<std::reference_wrapper<CMapOutdoor>> {
            if (mapMgr.IsMapOutdoor()) {
                return mapMgr.GetMapOutdoorRef();
            }
            return std::nullopt;
        };

        return getOutdoorMap()
            .transform([&](std::reference_wrapper<CMapOutdoor> outdoor) -> std::expected<D3DXVECTOR3, EterBase::NavigationError> {
                EterBase::ModernLogger::Debug("Starting raymarch from ({}, {}, {}) dir ({}, {}, {}) max_dist: {}",
                                              origin.x, origin.y, origin.z,
                                              direction.x, direction.y, direction.z, maxDistance);

                // We use half a cell size as the raymarch step to ensure we don't skip over terrain peaks.
                constexpr float STEP_SIZE = static_cast<float>(CTerrainImpl::HALF_CELLSCALE);
                float currentDistance = 0.0f;
                
                while (currentDistance <= maxDistance) {
                    D3DXVECTOR3 currentPos = origin + direction * currentDistance;
                    float terrainHeight = outdoor.get().GetTerrainHeight(currentPos.x, currentPos.y);
                    
                    if (currentPos.z <= terrainHeight) {
                        // Ray has intersected or gone below the terrain
                        EterBase::ModernLogger::Debug("Raymarch hit terrain at ({}, {}, {})", currentPos.x, currentPos.y, currentPos.z);
                        UserInterface::Core::EventBus::GetInstance().Publish(RaymarchHitEvent{currentPos});
                        return currentPos;
                    }
                    
                    currentDistance += STEP_SIZE;
                }
                
                // Check exact end point to be precise
                D3DXVECTOR3 endPos = origin + direction * maxDistance;
                float terrainHeight = outdoor.get().GetTerrainHeight(endPos.x, endPos.y);
                
                if (endPos.z <= terrainHeight) {
                    EterBase::ModernLogger::Debug("Raymarch hit terrain at end pos ({}, {}, {})", endPos.x, endPos.y, endPos.z);
                    UserInterface::Core::EventBus::GetInstance().Publish(RaymarchHitEvent{endPos});
                    return endPos;
                }

                EterBase::ModernLogger::Debug("Raymarch finished with no hits.");
                return std::unexpected(EterBase::NavigationError::PathNotFound);
            })
            .value_or(std::unexpected(EterBase::NavigationError::MapNotLoaded));
    }
};

} // namespace GameLib
