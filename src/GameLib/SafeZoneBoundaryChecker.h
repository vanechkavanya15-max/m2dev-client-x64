#pragma once

#include <cstdint>
#include <optional>
#include <cmath>
#include "../EterBase/StrongTypes.h"
#include "../EterBase/Result.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Event emitted when an entity enters the safe zone.
 */
struct SafeZoneEnteredEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;

    /**
     * @brief Constructs the event.
     * @param id The unique identifier of the entity that entered the safe zone.
     */
    explicit SafeZoneEnteredEvent(EterBase::EntityId id) : entityId(id) {}
};

/**
 * @brief Event emitted when an entity exits the safe zone.
 */
struct SafeZoneExitedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;

    /**
     * @brief Constructs the event.
     * @param id The unique identifier of the entity that exited the safe zone.
     */
    explicit SafeZoneExitedEvent(EterBase::EntityId id) : entityId(id) {}
};

/**
 * @brief Defines a circular safe zone area in the game world.
 */
struct SafeZone {
    float centerX{0.0f};
    float centerY{0.0f};
    float radius{0.0f};

    /**
     * @brief Checks if a given coordinate is within this safe zone.
     * @param x The X coordinate.
     * @param y The Y coordinate.
     * @return True if within the safe zone, false otherwise.
     */
    [[nodiscard]] constexpr bool Contains(float x, float y) const noexcept {
        float dx = x - centerX;
        float dy = y - centerY;
        return (dx * dx + dy * dy) <= (radius * radius);
    }
};

/**
 * @brief Class responsible for tracking and validating safe zone boundaries for entities.
 * 
 * Uses C++23 features like `std::expected` and `std::optional` for safety and elegance.
 * Completely decoupled from the GUI, state changes trigger events via the EventBus.
 */
class SafeZoneBoundaryChecker {
public:
    SafeZoneBoundaryChecker() = default;

    /**
     * @brief Sets the active safe zone.
     * @param zone The new safe zone to monitor.
     */
    void SetSafeZone(const SafeZone& zone) {
        m_safeZone = zone;
        EterBase::ModernLogger::Info("SafeZoneBoundaryChecker: Safe zone updated (X:{}, Y:{}, R:{})", 
                                     zone.centerX, zone.centerY, zone.radius);
    }

    /**
     * @brief Clears the active safe zone.
     */
    void ClearSafeZone() {
        m_safeZone.reset();
        EterBase::ModernLogger::Info("SafeZoneBoundaryChecker: Safe zone cleared");
    }

    /**
     * @brief Checks if a point is within the safe zone and processes state transitions for an entity.
     * 
     * Uses C++23 monadic operations to elegantly check the zone and dispatch events.
     * 
     * @param id The entity ID to check.
     * @param x The current X coordinate of the entity.
     * @param y The current Y coordinate of the entity.
     * @param wasInSafeZone True if the entity was previously in the safe zone.
     * @return A Result indicating the new safe zone state (true if inside, false if outside) or a NavigationError if no safe zone is active.
     */
    [[nodiscard]] EterBase::Result<bool, EterBase::NavigationError> UpdateEntityState(
        EterBase::EntityId id, float x, float y, bool wasInSafeZone) const 
    {
        if (!m_safeZone) {
            return std::unexpected(EterBase::NavigationError::MapNotLoaded);
        }

        return m_safeZone
            .transform([x, y](const SafeZone& zone) {
                return zone.Contains(x, y);
            })
            .transform([id, wasInSafeZone](bool isCurrentlyInSafeZone) {
                if (isCurrentlyInSafeZone && !wasInSafeZone) {
                    EterBase::ModernLogger::Debug("Entity {} entered safe zone", id);
                    UserInterface::Core::EventBus::GetInstance().Publish(SafeZoneEnteredEvent{id});
                } else if (!isCurrentlyInSafeZone && wasInSafeZone) {
                    EterBase::ModernLogger::Debug("Entity {} exited safe zone", id);
                    UserInterface::Core::EventBus::GetInstance().Publish(SafeZoneExitedEvent{id});
                }
                return isCurrentlyInSafeZone;
            })
            .value();
    }

private:
    std::optional<SafeZone> m_safeZone;
};

} // namespace GameLib
