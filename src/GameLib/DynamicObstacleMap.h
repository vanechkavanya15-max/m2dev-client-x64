#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <cmath>

#include "src/GameLib/CoordTransformer.h"
#include "src/GameLib/DistanceMatrix.h"
#include "src/EterBase/StrongTypes.h"
#include "src/EterBase/Result.h"
#include "src/EterBase/LogModern.h"
#include "src/UserInterface/Core/EventBus.h"

namespace Navigation {

/**
 * @struct DynamicObstacleAddedEvent
 * @brief Event emitted when a new dynamic obstacle is added to the map.
 */
struct DynamicObstacleAddedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId id;
    PixelCoord center;
    float radius;

    /**
     * @brief Constructs the event.
     * @param id The unique identifier of the obstacle.
     * @param center The pixel coordinates of the obstacle's center.
     * @param radius The radius of the obstacle.
     */
    DynamicObstacleAddedEvent(EterBase::EntityId id, PixelCoord center, float radius)
        : id(id), center(center), radius(radius) {}
};

/**
 * @struct DynamicObstacleRemovedEvent
 * @brief Event emitted when a dynamic obstacle is removed from the map.
 */
struct DynamicObstacleRemovedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId id;

    /**
     * @brief Constructs the event.
     * @param id The unique identifier of the obstacle to remove.
     */
    explicit DynamicObstacleRemovedEvent(EterBase::EntityId id) : id(id) {}
};

/**
 * @struct DynamicObstacleUpdatedEvent
 * @brief Event emitted when a dynamic obstacle's position is updated.
 */
struct DynamicObstacleUpdatedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId id;
    PixelCoord newCenter;

    /**
     * @brief Constructs the event.
     * @param id The unique identifier of the obstacle.
     * @param newCenter The new pixel coordinates of the obstacle's center.
     */
    DynamicObstacleUpdatedEvent(EterBase::EntityId id, PixelCoord newCenter)
        : id(id), newCenter(newCenter) {}
};

/**
 * @struct DynamicObstacle
 * @brief Represents a temporary, dynamic obstacle in the game world (e.g., players, metin stones).
 */
struct DynamicObstacle {
    EterBase::EntityId id;
    PixelCoord center;
    float radius;
};

/**
 * @class DynamicObstacleMap
 * @brief Thread-safe manager for dynamic obstacles like players, NPCs, and metin stones.
 * 
 * This class provides mechanisms to add, remove, and check collisions against temporary obstacles.
 * It strictly decouples state updates by emitting events via the EventBus for the UI or other systems to react.
 */
class DynamicObstacleMap {
public:
    DynamicObstacleMap() = default;
    ~DynamicObstacleMap() = default;

    DynamicObstacleMap(const DynamicObstacleMap&) = delete;
    DynamicObstacleMap& operator=(const DynamicObstacleMap&) = delete;
    DynamicObstacleMap(DynamicObstacleMap&&) = delete;
    DynamicObstacleMap& operator=(DynamicObstacleMap&&) = delete;

    /**
     * @brief Adds a new dynamic obstacle to the map.
     * 
     * @param id The unique identifier of the obstacle.
     * @param center The initial pixel coordinates.
     * @param radius The collision radius of the obstacle.
     * @return EterBase::VoidResult<EterBase::EntityError> Success or an error indicating failure (e.g., already exists).
     */
    EterBase::VoidResult<EterBase::EntityError> AddObstacle(EterBase::EntityId id, PixelCoord center, float radius) {
        {
            std::unique_lock lock(mutex_);
            if (obstacles_.contains(id)) {
                EterBase::ModernLogger::Warn("Failed to add obstacle: Entity {} already exists.", id);
                return EterBase::MakeError(EterBase::EntityError::AlreadyExists);
            }
            obstacles_.emplace(id, DynamicObstacle{id, center, radius});
        }
        
        EterBase::ModernLogger::Debug("Added dynamic obstacle for Entity {} at ({}, {}).", id, center.x, center.y);
        UserInterface::Core::EventBus::GetInstance().Publish(DynamicObstacleAddedEvent{id, center, radius});

        return {};
    }

    /**
     * @brief Removes a dynamic obstacle from the map.
     * 
     * @param id The unique identifier of the obstacle to remove.
     * @return EterBase::VoidResult<EterBase::EntityError> Success or an error indicating failure (e.g., not found).
     */
    EterBase::VoidResult<EterBase::EntityError> RemoveObstacle(EterBase::EntityId id) {
        {
            std::unique_lock lock(mutex_);
            if (!obstacles_.erase(id)) {
                EterBase::ModernLogger::Warn("Failed to remove obstacle: Entity {} not found.", id);
                return EterBase::MakeError(EterBase::EntityError::NotFound);
            }
        }

        EterBase::ModernLogger::Debug("Removed dynamic obstacle for Entity {}.", id);
        UserInterface::Core::EventBus::GetInstance().Publish(DynamicObstacleRemovedEvent{id});

        return {};
    }

    /**
     * @brief Updates the position of an existing dynamic obstacle.
     * 
     * @param id The unique identifier of the obstacle.
     * @param newCenter The new pixel coordinates.
     * @return EterBase::VoidResult<EterBase::EntityError> Success or an error indicating failure (e.g., not found).
     */
    EterBase::VoidResult<EterBase::EntityError> UpdateObstaclePosition(EterBase::EntityId id, PixelCoord newCenter) {
        {
            std::unique_lock lock(mutex_);
            auto it = obstacles_.find(id);
            if (it == obstacles_.end()) {
                EterBase::ModernLogger::Warn("Failed to update obstacle position: Entity {} not found.", id);
                return EterBase::MakeError(EterBase::EntityError::NotFound);
            }
            it->second.center = newCenter;
        }
        
        UserInterface::Core::EventBus::GetInstance().Publish(DynamicObstacleUpdatedEvent{id, newCenter});

        return {};
    }

    /**
     * @brief Checks if a given 2D pixel coordinate is blocked by any obstacle.
     * 
     * @param point The point in pixel coordinates to test.
     * @return true If the point lies within the radius of any registered obstacle.
     * @return false If the point is unblocked.
     */
    [[nodiscard]] bool IsPointBlocked(PixelCoord point) const {
        std::shared_lock lock(mutex_);
        
        for (const auto& [id, obstacle] : obstacles_) {
            if (::DistanceMatrix::IsInRange2D(point.x, point.y, obstacle.center.x, obstacle.center.y, obstacle.radius)) {
                return true;
            }
        }
        
        return false;
    }

    /**
     * @brief Checks if a given server coordinate is blocked.
     * 
     * Safely converts the server coordinate to a pixel coordinate, then evaluates blocking.
     * 
     * @param serverCoord The point in server coordinates to test.
     * @return true If the point is blocked.
     * @return false If the point is unblocked or the coordinate conversion fails.
     */
    [[nodiscard]] bool IsServerCoordBlocked(ServerCoord serverCoord) const {
        return CoordTransformer::ServerToPixel(serverCoord)
            .transform([this](const PixelCoord& pixelCoord) {
                return IsPointBlocked(pixelCoord);
            })
            .value_or(false);
    }

    /**
     * @brief Finds the specific obstacle blocking a given point, if any.
     * 
     * @param point The pixel coordinates to test.
     * @return std::optional<DynamicObstacle> The blocking obstacle, or std::nullopt if unblocked.
     */
    [[nodiscard]] std::optional<DynamicObstacle> GetBlockingObstacle(PixelCoord point) const {
        std::shared_lock lock(mutex_);
        
        for (const auto& [id, obstacle] : obstacles_) {
            if (::DistanceMatrix::IsInRange2D(point.x, point.y, obstacle.center.x, obstacle.center.y, obstacle.radius)) {
                return obstacle;
            }
        }
        
        return std::nullopt;
    }

    /**
     * @brief Clears all registered dynamic obstacles from the map.
     */
    void Clear() {
        std::unique_lock lock(mutex_);
        obstacles_.clear();
        EterBase::ModernLogger::Info("Cleared all dynamic obstacles.");
    }

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<EterBase::EntityId, DynamicObstacle> obstacles_;
};

} // namespace Navigation
