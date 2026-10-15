#pragma once

#include <vector>
#include <optional>
#include <format>
#include <EterBase/Result.h>
#include <EterBase/StrongTypes.h>
#include <EterBase/LogModern.h>
#include <UserInterface/Core/EventBus.h>

namespace GameLib::Navigation {

/**
 * @brief Represents a rectangular water body that blocks land movement.
 */
struct WaterZone {
    int startX;
    int startY;
    int endX;
    int endY;
};

/**
 * @brief Event emitted when an entity's movement is blocked by a water hazard.
 */
struct WaterBlockEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    int attemptedX;
    int attemptedY;

    /**
     * @brief Constructs a WaterBlockEvent.
     * @param id The ID of the entity that was blocked.
     * @param x The X coordinate of the attempted movement.
     * @param y The Y coordinate of the attempted movement.
     */
    WaterBlockEvent(EterBase::EntityId id, int x, int y)
        : entityId(id), attemptedX(x), attemptedY(y) {}
};

/**
 * @brief Manager for detecting and handling water hazards to prevent land movement.
 */
class WaterHazardMap {
public:
    /**
     * @brief Adds a new water zone to the hazard map.
     * @param startX Minimum X coordinate.
     * @param startY Minimum Y coordinate.
     * @param endX Maximum X coordinate.
     * @param endY Maximum Y coordinate.
     */
    void AddWaterZone(int startX, int startY, int endX, int endY) {
        zones.push_back({startX, startY, endX, endY});
        EterBase::ModernLogger::Info("Added WaterZone [{} {}] to [{} {}]", startX, startY, endX, endY);
    }

    /**
     * @brief Clears all registered water zones.
     */
    void ClearZones() {
        zones.clear();
        EterBase::ModernLogger::Info("Cleared all WaterZones.");
    }

    /**
     * @brief Finds the first water zone that encompasses the given coordinates.
     * @param x The X coordinate to check.
     * @param y The Y coordinate to check.
     * @return Optional containing the blocking WaterZone if found, std::nullopt otherwise.
     */
    std::optional<WaterZone> FindBlockingZone(int x, int y) const {
        for (const auto& zone : zones) {
            if (x >= zone.startX && x <= zone.endX && y >= zone.startY && y <= zone.endY) {
                return zone;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Checks if a specific location is free of water hazards using monadic operations.
     * @param x The X coordinate to check.
     * @param y The Y coordinate to check.
     * @return true if movement is allowed (no water), false otherwise.
     */
    bool CanMoveTo(int x, int y) const {
        return !FindBlockingZone(x, y)
                .transform([](const WaterZone&) { return true; })
                .value_or(false);
    }

    /**
     * @brief Attempts a movement for an entity, checking against water hazards.
     * @param entityId The ID of the entity attempting to move.
     * @param x The target X coordinate.
     * @param y The target Y coordinate.
     * @return Result containing void on success, or a NavigationError on failure.
     */
    EterBase::Result<void, EterBase::NavigationError> CheckMovement(EterBase::EntityId entityId, int x, int y) const {
        if (!CanMoveTo(x, y)) {
            EterBase::ModernLogger::Debug("Entity {} blocked by water at ({}, {})", entityId.value(), x, y);
            UserInterface::Core::EventBus::GetInstance().Publish(WaterBlockEvent(entityId, x, y));
            return std::unexpected(EterBase::NavigationError::BlockedTerrain);
        }
        return {};
    }

private:
    std::vector<WaterZone> zones;
};

} // namespace GameLib::Navigation
