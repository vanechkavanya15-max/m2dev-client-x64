#pragma once

#include <cmath>
#include <numbers>
#include <cstdint>
#include <algorithm>

namespace Movement {

/**
 * @brief Represents a 2D point or vector in the game world.
 */
struct Point2D {
    float x;
    float y;
};

/**
 * @brief Helper class for rotation and angle calculations.
 */
class RotationHelper {
public:
    /**
     * @brief Normalizes an angle to the [0.0, 360.0) range.
     * @param angle The angle in degrees.
     * @return The normalized angle in degrees.
     */
    static constexpr float NormalizeAngle(float angle) noexcept {
        while (angle >= 360.0f) {
            angle -= 360.0f;
        }
        while (angle < 0.0f) {
            angle += 360.0f;
        }
        return angle;
    }

    /**
     * @brief Calculates the angle in degrees from a direction vector.
     * @param direction The direction vector (x, y).
     * @return The angle in degrees [0.0, 360.0).
     */
    static inline float GetAngleFromDirection(const Point2D& direction) noexcept {
        // Metin2 standard direction is (0, -1) which corresponds to 0 degrees.
        // atan2(x, -y) maps (0, -1) to 0, (1, 0) to pi/2, etc.
        const float radians = std::atan2(direction.x, -direction.y);
        const float degrees = radians * (180.0f / std::numbers::pi_v<float>);
        return NormalizeAngle(degrees);
    }

    /**
     * @brief Calculates the angle in degrees between two points.
     * @param source The starting point.
     * @param target The target point.
     * @return The angle in degrees [0.0, 360.0) from source to target.
     */
    static inline float GetAngleBetweenPoints(const Point2D& source, const Point2D& target) noexcept {
        const Point2D direction = { target.x - source.x, target.y - source.y };
        return GetAngleFromDirection(direction);
    }

    /**
     * @brief Calculates the absolute minimal difference between two angles.
     * @param sourceAngle The starting angle in degrees.
     * @param targetAngle The target angle in degrees.
     * @return The absolute difference in degrees [0.0, 180.0].
     */
    static constexpr float GetAngleDifference(float sourceAngle, float targetAngle) noexcept {
        float difference = std::abs(NormalizeAngle(targetAngle) - NormalizeAngle(sourceAngle));
        if (difference > 180.0f) {
            difference = 360.0f - difference;
        }
        return difference;
    }

    /**
     * @brief Calculates the signed difference between two angles.
     * @param sourceAngle The starting angle in degrees.
     * @param targetAngle The target angle in degrees.
     * @return The signed difference in degrees [-180.0, 180.0]. Positive means clockwise rotation.
     */
    static constexpr float GetSignedAngleDifference(float sourceAngle, float targetAngle) noexcept {
        float difference = NormalizeAngle(targetAngle) - NormalizeAngle(sourceAngle);
        if (difference > 180.0f) {
            difference -= 360.0f;
        } else if (difference < -180.0f) {
            difference += 360.0f;
        }
        return difference;
    }
};

} // namespace Movement
