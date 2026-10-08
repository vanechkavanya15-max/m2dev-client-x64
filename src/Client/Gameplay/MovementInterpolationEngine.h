#pragma once

#include <cstdint>
#include <vector>
#include <mutex>
#include <optional>
#include "Client/Core/StrongTypes.h"

namespace Client::Gameplay {

/**
 * @brief Handles dead-reckoning and interpolation (LERP for position, SLERP for rotation)
 *        for external entities based on network updates.
 */
class MovementInterpolationEngine {
public:
    struct Snapshot {
        Client::Core::MapCoords position;
        Client::Core::MapCoords velocity;
        float rotation; // in degrees
        uint64_t timestamp;
    };

    MovementInterpolationEngine() = default;
    ~MovementInterpolationEngine() = default;

    /**
     * @brief Updates the engine with a new network snapshot.
     */
    void AddSnapshot(const Client::Core::MapCoords& position,
                     const Client::Core::MapCoords& velocity,
                     float rotation,
                     uint64_t timestamp);

    /**
     * @brief Calculates the interpolated position using LERP or dead-reckoning.
     * @param timestamp The current time to interpolate/extrapolate to.
     */
    Client::Core::MapCoords GetInterpolatedPosition(uint64_t timestamp) const;

    /**
     * @brief Calculates the interpolated rotation using SLERP.
     * @param timestamp The current time to interpolate/extrapolate to.
     */
    float GetInterpolatedRotation(uint64_t timestamp) const;

    /**
     * @brief Clears all history.
     */
    void Clear();

private:
    float SlerpRotation(float currentYaw, float targetYaw, float alpha) const;

    mutable std::mutex m_mutex;
    std::vector<Snapshot> m_snapshots;
};

} // namespace Client::Gameplay
