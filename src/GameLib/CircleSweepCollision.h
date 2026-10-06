#pragma once

#include <expected>
#include <optional>
#include <cmath>
#include <tuple>
#include "../EterBase/StrongTypes.h"
#include "../EterBase/Result.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Event triggered when a sweep collision occurs.
 * 
 * This event decoupled the collision logic from the GUI, enabling
 * components to subscribe to collision events through the EventBus.
 */
struct CollisionSweepEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    EterBase::EntityId targetId;
    float collisionTime;

    /**
     * @brief Constructs a new CollisionSweepEvent.
     * @param entity The moving entity ID.
     * @param target The stationary/target entity ID.
     * @param time The collision time normalized to [0, 1].
     */
    CollisionSweepEvent(EterBase::EntityId entity, EterBase::EntityId target, float time)
        : entityId(entity), targetId(target), collisionTime(time) {}
};

/**
 * @brief Continuous collision detection namespace for swept circle/capsule tests.
 */
namespace CircleSweepCollision {

    /**
     * @brief Performs a swept circle collision test against a stationary circle.
     * 
     * Tests whether a moving circle (from start to end) intersects with a stationary circle.
     * This is useful for testing capsule-like movement traces.
     * 
     * @param entityId The identifier of the moving entity.
     * @param targetId The identifier of the target entity.
     * @param startX Starting X coordinate of the moving entity.
     * @param startY Starting Y coordinate of the moving entity.
     * @param endX Ending X coordinate of the moving entity.
     * @param endY Ending Y coordinate of the moving entity.
     * @param radius The radius of the moving entity (capsule radius).
     * @param targetX The X coordinate of the target entity.
     * @param targetY The Y coordinate of the target entity.
     * @param targetRadius The radius of the target entity.
     * 
     * @return std::expected<float, EterBase::NavigationError> The collision time (0.0 to 1.0) if a collision occurs,
     *         or EterBase::NavigationError::PathNotFound if no collision occurs.
     */
    inline std::expected<float, EterBase::NavigationError> TestCollision(
        EterBase::EntityId entityId, EterBase::EntityId targetId,
        float startX, float startY, float endX, float endY, float radius,
        float targetX, float targetY, float targetRadius) 
    {
        // Vector from start to end (movement vector)
        const float dx = endX - startX;
        const float dy = endY - startY;

        // Vector from start to target center
        const float fx = startX - targetX;
        const float fy = startY - targetY;

        // Combined radius
        const float combinedRadius = radius + targetRadius;
        
        // Quadratic coefficients: a*t^2 + b*t + c = 0
        const float a = (dx * dx) + (dy * dy);
        const float b = 2.0f * ((fx * dx) + (fy * dy));
        const float c = (fx * fx) + (fy * fy) - (combinedRadius * combinedRadius);

        // Check if initially overlapping
        if (c <= 0.0f) {
            UserInterface::Core::EventBus::GetInstance().Publish(CollisionSweepEvent(entityId, targetId, 0.0f));
            return 0.0f;
        }

        // If a is near zero, the entity didn't move
        if (std::abs(a) < 1e-6f) {
            return std::unexpected(EterBase::NavigationError::PathNotFound);
        }

        const float discriminant = (b * b) - (4.0f * a * c);

        // Negative discriminant means no intersection
        if (discriminant < 0.0f) {
            return std::unexpected(EterBase::NavigationError::PathNotFound);
        }

        // Ray didn't totally miss, calculate intersection times
        const float sqrtDiscriminant = std::sqrt(discriminant);
        const float t1 = (-b - sqrtDiscriminant) / (2.0f * a);
        const float t2 = (-b + sqrtDiscriminant) / (2.0f * a);

        std::optional<float> collisionTime;

        if (t1 >= 0.0f && t1 <= 1.0f) {
            collisionTime = t1;
        } else if (t2 >= 0.0f && t2 <= 1.0f) {
            collisionTime = t2;
        }

        return collisionTime
            .transform([&](float t) {
                UserInterface::Core::EventBus::GetInstance().Publish(CollisionSweepEvent(entityId, targetId, t));
                return t;
            })
            .transform([](float t) -> std::expected<float, EterBase::NavigationError> {
                return t;
            })
            .value_or(std::unexpected(EterBase::NavigationError::PathNotFound));
    }
} // namespace CircleSweepCollision
} // namespace GameLib
