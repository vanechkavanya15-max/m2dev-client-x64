#pragma once

#include <cstdint>
#include <optional>
#include <cmath>

namespace Movement {

/**
 * @brief Represents a 2D coordinate in the game world.
 */
struct Position {
    int32_t x;
    int32_t y;
};

/**
 * @brief Validates movement steps to prevent invalid or excessively large jumps.
 * 
 * The StepValidator ensures that character movements requested by the client
 * do not exceed a permissible maximum distance threshold in a single step.
 */
class StepValidator {
public:
    /**
     * @brief Constructs a new StepValidator with a specific distance limit.
     * 
     * @param limit The maximum allowed distance a character can move in one step.
     */
    explicit StepValidator(double limit = 1000.0) : maxDistanceLimit(limit) {}

    /**
     * @brief Validates if the movement from the last known position to the new position is valid.
     * 
     * If there is no previous position tracked, the movement is considered valid and the
     * new position is stored. Subsequent calls will validate against the last stored position.
     * 
     * @param currentX The current/new X coordinate.
     * @param currentY The current/new Y coordinate.
     * @return true if the distance between the last position and the new position is within the limit, false otherwise.
     */
    bool ValidateStep(int32_t currentX, int32_t currentY) {
        if (!lastPosition.has_value()) {
            lastPosition = Position{currentX, currentY};
            return true;
        }

        double distance = CalculateDistance(lastPosition->x, lastPosition->y, currentX, currentY);
        if (distance > maxDistanceLimit) {
            return false;
        }

        lastPosition = Position{currentX, currentY};
        return true;
    }

    /**
     * @brief Resets the validator, clearing the last known position.
     */
    void Reset() {
        lastPosition.reset();
    }

    /**
     * @brief Sets a new maximum distance limit.
     * 
     * @param limit The new maximum allowed distance a character can move in one step.
     */
    void SetLimit(double limit) {
        maxDistanceLimit = limit;
    }

private:
    /**
     * @brief Calculates the Euclidean distance between two 2D points.
     * 
     * @param startX The starting X coordinate.
     * @param startY The starting Y coordinate.
     * @param endX The ending X coordinate.
     * @param endY The ending Y coordinate.
     * @return double The calculated distance.
     */
    double CalculateDistance(int32_t startX, int32_t startY, int32_t endX, int32_t endY) const {
        double dx = static_cast<double>(endX - startX);
        double dy = static_cast<double>(endY - startY);
        return std::sqrt(dx * dx + dy * dy);
    }

    std::optional<Position> lastPosition; ///< The last recorded valid position.
    double maxDistanceLimit;              ///< The maximum allowed distance per step.
};

} // namespace Movement
