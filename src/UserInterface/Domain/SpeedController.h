#pragma once

#include <cstdint>
#include <cmath>
#include <limits>
#include <optional>

/**
 * @brief Represents a 2D coordinate in the game world.
 */
struct Position {
    float x;
    float y;

    /**
     * @brief Computes the Euclidean distance to another position.
     * @param target The destination position.
     * @return The distance as a float.
     */
    float DistanceTo(const Position& target) const {
        float dx = x - target.x;
        float dy = y - target.y;
        return std::sqrt(dx * dx + dy * dy);
    }
};

/**
 * @brief A pure domain calculator for movement speed and arrival time.
 * 
 * Complies with C++20 guidelines:
 * - Zero UI coupling
 * - Zero Hungarian notation
 * - Single Responsibility Principle
 */
class SpeedController {
public:
    /**
     * @brief Constructs a new SpeedController.
     * @param initialSpeed The starting movement speed (units per second).
     */
    explicit SpeedController(float initialSpeed = 0.0f) 
        : speed(initialSpeed) {}

    /**
     * @brief Sets a new movement speed.
     * @param newSpeed The new speed in units per second.
     */
    void SetSpeed(float newSpeed) {
        if (newSpeed >= 0.0f) {
            speed = newSpeed;
        }
    }

    /**
     * @brief Gets the current movement speed.
     * @return The current speed in units per second.
     */
    float GetSpeed() const {
        return speed;
    }

    /**
     * @brief Calculates the time required to travel from a start position to a target position.
     * 
     * @param start The starting coordinate.
     * @param target The destination coordinate.
     * @return The estimated time in seconds. Returns std::nullopt if speed is 0.
     */
    std::optional<float> CalculateArrivalTime(const Position& start, const Position& target) const {
        if (speed <= 0.0f) {
            return std::nullopt;
        }

        float distance = start.DistanceTo(target);
        return distance / speed;
    }

private:
    float speed;
};
