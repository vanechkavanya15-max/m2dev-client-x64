#pragma once

#include <vector>
#include <unordered_map>
#include <optional>
#include <algorithm>
#include <cmath>
#include <expected>

#include "../EterBase/StrongTypes.h"
#include "../EterBase/Result.h"
#include "../EterBase/LogModern.h"

namespace Core {
    struct IEvent {
        virtual ~IEvent() = default;
    };

    class EventBus {
    public:
        static EventBus& Instance();
        template <typename EventType>
        void Publish(const EventType& event);
    };
}

namespace GameLib::Navigation {

/**
 * @brief Represents a point in 3D space.
 */
struct Vector3D {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    Vector3D& operator+=(const Vector3D& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3D& operator-=(const Vector3D& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vector3D& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }
    
    Vector3D& operator/=(float scalar) {
        if (scalar != 0.0f) {
            x /= scalar;
            y /= scalar;
            z /= scalar;
        }
        return *this;
    }

    [[nodiscard]] float Length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    [[nodiscard]] float DistanceTo(const Vector3D& other) const {
        return std::sqrt(std::pow(x - other.x, 2) + std::pow(y - other.y, 2) + std::pow(z - other.z, 2));
    }

    void Normalize() {
        float len = Length();
        if (len > 0.0f) {
            x /= len;
            y /= len;
            z /= len;
        }
    }
};

/**
 * @brief Event triggered when a flocking entity needs to move to a new position.
 * This decouples the flocking logic from the actual rendering and physics updates.
 */
struct FlockingMoveEvent : public Core::IEvent {
    EterBase::EntityId entityId;
    Vector3D newPosition;
    Vector3D newVelocity;

    /**
     * @brief Constructs a new Flocking Move Event.
     * @param id The ID of the entity that moved.
     * @param position The new position of the entity.
     * @param velocity The new velocity vector of the entity.
     */
    FlockingMoveEvent(EterBase::EntityId id, const Vector3D& position, const Vector3D& velocity)
        : entityId(id), newPosition(position), newVelocity(velocity) {}
};

/**
 * @brief Represents an entity participating in flocking behavior.
 */
struct FlockingEntity {
    EterBase::EntityId id;
    Vector3D position;
    Vector3D velocity;
    float speed;
};

/**
 * @brief Coordinates movement of a group of entities (bots/mobs) using flocking behavior.
 * 
 * Implements Reynolds' flocking algorithm:
 * - Separation: steer to avoid crowding local flockmates.
 * - Alignment: steer towards the average heading of local flockmates.
 * - Cohesion: steer to move toward the average position of local flockmates.
 */
class FlockingBehaviorCoordinator {
public:
    /**
     * @brief Adds an entity to the flocking coordinator.
     * @param entityId The unique identifier of the entity.
     * @param position The initial position of the entity.
     * @param velocity The initial velocity of the entity.
     * @param speed The movement speed of the entity.
     * @return std::expected<void, EterBase::NavigationError> Success or error if entity already exists.
     */
    std::expected<void, EterBase::NavigationError> AddEntity(
        EterBase::EntityId entityId, 
        const Vector3D& position, 
        const Vector3D& velocity,
        float speed) 
    {
        if (entities_.contains(entityId)) {
            EterBase::ModernLogger::Warn("Entity {} already exists in flocking coordinator.", entityId.value());
            return std::unexpected(EterBase::NavigationError::DestinationUnreachable);
        }
        
        entities_.emplace(entityId, FlockingEntity{entityId, position, velocity, speed});
        EterBase::ModernLogger::Debug("Added entity {} to flock.", entityId.value());
        return {};
    }

    /**
     * @brief Removes an entity from the flocking coordinator.
     * @param entityId The unique identifier of the entity to remove.
     * @return std::expected<void, EterBase::NavigationError> Success or error if entity not found.
     */
    std::expected<void, EterBase::NavigationError> RemoveEntity(EterBase::EntityId entityId) {
        if (!entities_.erase(entityId)) {
            EterBase::ModernLogger::Warn("Failed to remove entity {}: not found.", entityId.value());
            return std::unexpected(EterBase::NavigationError::PathNotFound);
        }
        EterBase::ModernLogger::Debug("Removed entity {} from flock.", entityId.value());
        return {};
    }

    /**
     * @brief Updates the positions and velocities of all entities in the flock.
     * Calculates Separation, Alignment, and Cohesion forces, then publishes FlockingMoveEvent.
     * @param deltaTime The time elapsed since the last update.
     */
    void Update(float deltaTime) {
        std::unordered_map<EterBase::EntityId, Vector3D> newVelocities;

        for (const auto& [id, entity] : entities_) {
            Vector3D separationForce = CalculateSeparation(entity);
            Vector3D alignmentForce = CalculateAlignment(entity);
            Vector3D cohesionForce = CalculateCohesion(entity);

            // Weights for each behavior could be configurable, hardcoded for now
            separationForce *= 1.5f;
            alignmentForce *= 1.0f;
            cohesionForce *= 1.0f;

            Vector3D newVelocity = entity.velocity;
            newVelocity += separationForce;
            newVelocity += alignmentForce;
            newVelocity += cohesionForce;

            newVelocity.Normalize();
            newVelocity *= entity.speed;
            
            newVelocities[id] = newVelocity;
        }

        for (auto& [id, entity] : entities_) {
            entity.velocity = newVelocities[id];
            
            Vector3D deltaMove = entity.velocity;
            deltaMove *= deltaTime;
            entity.position += deltaMove;
            
            Core::EventBus::Instance().Publish(
                FlockingMoveEvent(entity.id, entity.position, entity.velocity)
            );
        }
    }

    /**
     * @brief Retrieves the current state of an entity.
     * @param entityId The unique identifier of the entity.
     * @return std::optional<FlockingEntity> The entity state if found, std::nullopt otherwise.
     */
    [[nodiscard]] std::optional<FlockingEntity> GetEntity(EterBase::EntityId entityId) const {
        if (auto it = entities_.find(entityId); it != entities_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    float neighborRadius_{500.0f};
    float separationRadius_{100.0f};

    std::unordered_map<EterBase::EntityId, FlockingEntity> entities_;

    [[nodiscard]] Vector3D CalculateSeparation(const FlockingEntity& entity) const {
        Vector3D steeringForce;
        int count = 0;

        for (const auto& [otherId, otherEntity] : entities_) {
            if (entity.id == otherId) continue;

            float distance = entity.position.DistanceTo(otherEntity.position);
            if (distance > 0 && distance < separationRadius_) {
                Vector3D diff = entity.position;
                diff -= otherEntity.position;
                diff.Normalize();
                diff /= distance; // Weight by distance
                steeringForce += diff;
                count++;
            }
        }

        if (count > 0) {
            steeringForce /= static_cast<float>(count);
        }
        return steeringForce;
    }

    [[nodiscard]] Vector3D CalculateAlignment(const FlockingEntity& entity) const {
        Vector3D averageVelocity;
        int count = 0;

        for (const auto& [otherId, otherEntity] : entities_) {
            if (entity.id == otherId) continue;

            float distance = entity.position.DistanceTo(otherEntity.position);
            if (distance > 0 && distance < neighborRadius_) {
                averageVelocity += otherEntity.velocity;
                count++;
            }
        }

        if (count > 0) {
            averageVelocity /= static_cast<float>(count);
            averageVelocity.Normalize();
        }
        return averageVelocity;
    }

    [[nodiscard]] Vector3D CalculateCohesion(const FlockingEntity& entity) const {
        Vector3D centerOfMass;
        int count = 0;

        for (const auto& [otherId, otherEntity] : entities_) {
            if (entity.id == otherId) continue;

            float distance = entity.position.DistanceTo(otherEntity.position);
            if (distance > 0 && distance < neighborRadius_) {
                centerOfMass += otherEntity.position;
                count++;
            }
        }

        if (count > 0) {
            centerOfMass /= static_cast<float>(count);
            
            // Steer towards center of mass
            Vector3D steeringForce = centerOfMass;
            steeringForce -= entity.position;
            steeringForce.Normalize();
            return steeringForce;
        }
        
        return Vector3D{};
    }
};

} // namespace GameLib::Navigation
