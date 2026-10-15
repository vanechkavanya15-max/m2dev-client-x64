#pragma once

#include <expected>
#include <cmath>
#include <format>
#include <optional>

#include "../EterBase/Result.h"
#include "../EterBase/StrongTypes.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Event emitted when a slope validation occurs.
 * 
 * Contains information regarding the entity requesting the move and
 * whether the slope validation was successful or not.
 */
struct SlopeValidationEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    float startZ;
    float targetZ;
    bool success;

    /**
     * @brief Constructs the slope validation event.
     * @param entityId The unique identifier of the entity attempting movement.
     * @param startZ The starting Z coordinate (height) of the entity.
     * @param targetZ The target Z coordinate (height) of the entity.
     * @param success Whether the entity successfully validated against the slope constraint.
     */
    SlopeValidationEvent(EterBase::EntityId entityId, float startZ, float targetZ, bool success)
        : entityId(entityId), startZ(startZ), targetZ(targetZ), success(success) {}
};

/**
 * @brief Terrain slope validator for determining navigable paths.
 * 
 * Uses modern C++23 features to strictly enforce zero-conflict, decoupled and
 * strongly typed navigation rules based on terrain geometry.
 */
class TerrainSlopeValidator {
public:
    /**
     * @brief Validates if the path between two points is navigable based on the slope angle.
     * 
     * Calculates the slope angle between the start and target coordinates and compares
     * it against the provided maxSlopeAngle. Logs a warning and publishes an event on failure.
     * 
     * @param entityId The strongly-typed unique identifier of the entity attempting the move.
     * @param startX Starting X coordinate in the world.
     * @param startY Starting Y coordinate in the world.
     * @param startZ Starting Z coordinate (height) in the world.
     * @param targetX Target X coordinate in the world.
     * @param targetY Target Y coordinate in the world.
     * @param targetZ Target Z coordinate (height) in the world.
     * @param maxSlopeAngle The maximum allowed slope angle in radians.
     * @return EterBase::Result<void, EterBase::NavigationError> indicating success or failure.
     */
    static std::expected<void, EterBase::NavigationError> ValidateSlope(
        EterBase::EntityId entityId,
        float startX, float startY, float startZ,
        float targetX, float targetY, float targetZ,
        float maxSlopeAngle) 
    {
        float dz = std::abs(targetZ - startZ);
        float distance = std::hypot(targetX - startX, targetY - startY);
        float slopeAngle = std::atan2(dz, distance);

        if (slopeAngle > maxSlopeAngle) {
            EterBase::ModernLogger::Log(
                EterBase::LogLevel::Warning,
                "Entity {0} failed slope validation. StartZ: {1}, TargetZ: {2}, Slope: {3}, MaxAllowed: {4}",
                entityId.value(), startZ, targetZ, slopeAngle, maxSlopeAngle
            );

            UserInterface::Core::EventBus::GetInstance().Publish(
                SlopeValidationEvent{entityId, startZ, targetZ, false}
            );

            return std::unexpected(EterBase::NavigationError::BlockedTerrain);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(
            SlopeValidationEvent{entityId, startZ, targetZ, true}
        );

        return {};
    }
};

} // namespace GameLib
