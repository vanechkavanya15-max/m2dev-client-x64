#pragma once

#include <cstdint>
#include <cmath>

/**
 * @brief Utility class for estimating the time required to complete motion.
 * 
 * The MotionTimeEstimator provides functionalities to calculate the time in 
 * milliseconds needed to travel a specific distance at a given speed. This is 
 * purely an internal state calculation and has no direct ties to the GUI.
 */
class MotionTimeEstimator {
public:
    /**
     * @brief Calculates the time in milliseconds required to cover a distance at a given speed.
     * 
     * @param distance The distance to be traveled (can be in any consistent unit).
     * @param speed The speed of movement (must be in distance-units per second).
     * @return uint32_t The estimated time in milliseconds. Returns 0 if distance <= 0.0f, 
     *         and returns UINT32_MAX if speed <= 0.0f to avoid division by zero or infinite time.
     */
    static uint32_t CalculateMotionTimeMs(float distance, float speed) {
        if (distance <= 0.0f) {
            return 0;
        }
        
        if (speed <= 0.0f) {
            // Speed is zero or negative, cannot reach destination (or invalid state).
            // Return maximum possible time as an error indicator or conceptually infinite time.
            return UINT32_MAX;
        }
        
        // Time in seconds = distance / speed
        float timeSeconds = distance / speed;
        
        // Convert to milliseconds
        float timeMilliseconds = timeSeconds * 1000.0f;
        
        // Handle potential overflow if time is extremely large
        if (timeMilliseconds >= static_cast<float>(UINT32_MAX)) {
            return UINT32_MAX;
        }
        
        return static_cast<uint32_t>(std::round(timeMilliseconds));
    }
};
