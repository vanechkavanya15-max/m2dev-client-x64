#pragma once

#include <expected>
#include <optional>
#include <chrono>
#include <format>
#include "../EterBase/StrongTypes.h"
#include "../EterBase/Result.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Event emitted when an actor successfully recovers from being stuck.
 */
struct StuckRecoveryEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    float newX;
    float newY;
    float newZ;

    /**
     * @brief Constructs the stuck recovery event.
     * @param id The entity ID that recovered.
     * @param x The new X coordinate.
     * @param y The new Y coordinate.
     * @param z The new Z coordinate.
     */
    StuckRecoveryEvent(EterBase::EntityId id, float x, float y, float z)
        : entityId(id), newX(x), newY(y), newZ(z) {}
};

/**
 * @brief Represents a coordinate in 3D space.
 */
struct Coordinate {
    float x;
    float y;
    float z;
};

/**
 * @brief Subsystem for detecting if an actor is stuck and attempting recovery.
 */
class StuckDetectionRecovery {
public:
    /**
     * @brief Constructor for StuckDetectionRecovery.
     */
    StuckDetectionRecovery() = default;
    
    /**
     * @brief Destructor for StuckDetectionRecovery.
     */
    ~StuckDetectionRecovery() = default;

    /**
     * @brief Detects if the entity is stuck.
     * @param entityId The ID of the entity to check.
     * @param currentCoord The current coordinates of the entity.
     * @param lastCoord The last known coordinates of the entity.
     * @return EterBase::Result<void, EterBase::NavigationError> indicating success or failure.
     */
    EterBase::Result<void, EterBase::NavigationError> DetectStuck(EterBase::EntityId entityId, const Coordinate& currentCoord, const std::optional<Coordinate>& lastCoord) {
        return lastCoord.and_then([&](const Coordinate& last) -> std::optional<EterBase::Result<void, EterBase::NavigationError>> {
            float dx = currentCoord.x - last.x;
            float dy = currentCoord.y - last.y;
            float dz = currentCoord.z - last.z;
            float distanceSq = dx * dx + dy * dy + dz * dz;
            if (distanceSq > 0.0f && distanceSq < 1.0f) { // If actively trying to move but distance moved is very small
                EterBase::ModernLogger::Warn("Entity {} might be stuck. Distance squared is {}.", entityId, distanceSq);
                return EterBase::MakeError(EterBase::NavigationError::BlockedTerrain);
            }
            return std::nullopt; // Not stuck
        }).value_or(EterBase::Result<void, EterBase::NavigationError>{});
    }

    /**
     * @brief Attempts to recover an entity from being stuck.
     * @param entityId The ID of the entity to recover.
     * @param currentCoord The current coordinates.
     * @return EterBase::Result<void, EterBase::NavigationError>
     */
    EterBase::Result<void, EterBase::NavigationError> AttemptRecovery(EterBase::EntityId entityId, const Coordinate& currentCoord) {
        // Find safe spot (simplified for example)
        auto safeSpot = FindSafeSpot(currentCoord);
        
        return safeSpot.transform([&](const Coordinate& safe) -> EterBase::Result<void, EterBase::NavigationError> {
            EterBase::ModernLogger::Info("Recovering entity {} to safe spot: ({}, {}, {})", entityId, safe.x, safe.y, safe.z);
            UserInterface::Core::EventBus::GetInstance().Publish(StuckRecoveryEvent(entityId, safe.x, safe.y, safe.z));
            return {};
        }).value_or(EterBase::MakeError(EterBase::NavigationError::DestinationUnreachable));
    }

private:
    /**
     * @brief Finds a nearby safe spot.
     * @param coord The current stuck coordinate.
     * @return std::optional<Coordinate> with the safe spot if found, std::nullopt otherwise.
     */
    std::optional<Coordinate> FindSafeSpot(const Coordinate& coord) {
        // Simplified dummy implementation
        return Coordinate{coord.x + 10.0f, coord.y + 10.0f, coord.z};
    }
};

} // namespace GameLib
