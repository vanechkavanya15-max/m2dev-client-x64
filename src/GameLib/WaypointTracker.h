#pragma once

#include <cstdint>
#include <vector>
#include <cmath>
#include <optional>
#include <span>

namespace Navigation {

/**
 * @brief Represents a single 3D point in the world for navigation.
 */
struct Waypoint {
    float x;
    float y;
    float z;
};

/**
 * @brief Manages a sequence of waypoints for entity navigation.
 *
 * The WaypointTracker is responsible for holding a path (sequence of waypoints)
 * and tracking the current progress along that path. It operates purely on
 * logical coordinates and is completely decoupled from any GUI or rendering logic.
 */
class WaypointTracker {
public:
    /**
     * @brief Constructs an empty WaypointTracker.
     */
    WaypointTracker() = default;

    /**
     * @brief Sets a new path for the tracker to follow.
     * 
     * @param newPath A span of waypoints representing the new path.
     */
    void SetPath(std::span<const Waypoint> newPath) {
        path.assign(newPath.begin(), newPath.end());
        currentIndex = 0;
    }

    /**
     * @brief Clears the current path, stopping navigation.
     */
    void ClearPath() {
        path.clear();
        currentIndex = 0;
    }

    /**
     * @brief Checks if there are any remaining waypoints in the current path.
     * 
     * @return true if the tracker has reached the end of the path or has no path, false otherwise.
     */
    [[nodiscard]] bool IsFinished() const {
        return currentIndex >= path.size();
    }

    /**
     * @brief Retrieves the next waypoint to navigate towards.
     * 
     * @return std::optional<Waypoint> The next waypoint, or std::nullopt if the path is finished.
     */
    [[nodiscard]] std::optional<Waypoint> GetCurrentWaypoint() const {
        if (IsFinished()) {
            return std::nullopt;
        }
        return path[currentIndex];
    }

    /**
     * @brief Updates the tracker's state based on the entity's current position.
     * 
     * If the entity has reached the current waypoint (within a specified tolerance),
     * the tracker advances to the next waypoint.
     * 
     * @param currentX The current X position of the entity.
     * @param currentY The current Y position of the entity.
     * @param currentZ The current Z position of the entity.
     * @param arrivalTolerance The maximum distance to the waypoint to be considered "arrived".
     * @return true if the waypoint was reached and the tracker advanced, false otherwise.
     */
    bool Update(float currentX, float currentY, float currentZ, float arrivalTolerance) {
        if (IsFinished()) {
            return false;
        }

        if (std::isnan(currentX) || std::isnan(currentY) || std::isnan(currentZ) || std::isnan(arrivalTolerance)) {
            return false;
        }

        const auto& target = path[currentIndex];
        
        float dx = target.x - currentX;
        float dy = target.y - currentY;
        float dz = target.z - currentZ;
        
        float distanceSquared = (dx * dx) + (dy * dy) + (dz * dz);
        
        if (distanceSquared <= (arrivalTolerance * arrivalTolerance)) {
            Advance();
            return true;
        }
        
        return false;
    }

    /**
     * @brief Manually advances the tracker to the next waypoint.
     */
    void Advance() {
        if (currentIndex < path.size()) {
            ++currentIndex;
        }
    }

    /**
     * @brief Gets the total number of waypoints in the current path.
     * 
     * @return uint32_t The total number of waypoints.
     */
    [[nodiscard]] uint32_t GetPathSize() const {
        return static_cast<uint32_t>(path.size());
    }

    /**
     * @brief Gets the index of the current waypoint being tracked.
     * 
     * @return uint32_t The current waypoint index.
     */
    [[nodiscard]] uint32_t GetCurrentIndex() const {
        return static_cast<uint32_t>(currentIndex);
    }

private:
    std::vector<Waypoint> path;
    size_t currentIndex{0};
};

} // namespace Navigation
