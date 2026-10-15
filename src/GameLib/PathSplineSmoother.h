#pragma once

#include <vector>
#include <expected>
#include <span>
#include <optional>
#include <format>
#include <cmath>
#include <cstdint>

#include "WaypointTracker.h"
#include "../EterBase/StrongTypes.h"
#include "../EterBase/LogModern.h"

// Forward declare Core::EventBus to avoid reverse dependencies on UserInterface.
// Assuming Core::EventBus is accessible in the final build environment.
namespace Core {
    class EventBus {
    public:
        static EventBus& Instance();
        template <typename T>
        void Publish(const T& event);
    };
}

namespace Navigation {

/**
 * @brief Enum representing potential errors during path smoothing.
 */
enum class SplineError {
    EmptyPath,
    InsufficientPoints,
    InvalidSubdivisions,
    CalculationError
};

/**
 * @brief Event published when a path is successfully smoothed.
 */
struct PathSmoothedEvent {
    EterBase::EntityId entityId;
    size_t originalPointCount;
    size_t smoothedPointCount;

    PathSmoothedEvent(EterBase::EntityId id, size_t origCount, size_t newCount)
        : entityId(id), originalPointCount(origCount), smoothedPointCount(newCount) {}
};

/**
 * @brief Smoothes angular paths using Catmull-Rom splines.
 *
 * This class takes an existing path (e.g., from A* Navigation) and generates a smoothed
 * version by interpolating points. It publishes events to the EventBus upon success.
 */
class PathSplineSmoother {
public:
    PathSplineSmoother() = default;
    ~PathSplineSmoother() = default;

    // Delete copy/move to prevent accidental copies if we ever hold state
    PathSplineSmoother(const PathSplineSmoother&) = delete;
    PathSplineSmoother& operator=(const PathSplineSmoother&) = delete;
    PathSplineSmoother(PathSplineSmoother&&) = delete;
    PathSplineSmoother& operator=(PathSplineSmoother&&) = delete;

    /**
     * @brief Generates a smoothed path using Catmull-Rom spline interpolation.
     * 
     * @param entityId The ID of the entity this path belongs to, used for event firing.
     * @param inputPath A span containing the original, coarse path waypoints.
     * @param subdivisions The number of segments to generate between each original pair of points. Must be > 0.
     * @return std::expected<std::vector<Waypoint>, SplineError> A smoothed vector of waypoints on success, or an error code.
     */
    [[nodiscard]] std::expected<std::vector<Waypoint>, SplineError> SmoothPath(
        EterBase::EntityId entityId,
        std::span<const Waypoint> inputPath, 
        uint32_t subdivisions) const 
    {
        if (inputPath.empty()) {
            EterBase::ModernLogger::Warn("PathSplineSmoother: Entity {} provided an empty path.", entityId.value());
            return std::unexpected(SplineError::EmptyPath);
        }

        if (inputPath.size() < 2) {
            // No need to smooth a single point, just return it.
            std::vector<Waypoint> singlePointPath;
            singlePointPath.push_back(inputPath[0]);
            return singlePointPath;
        }

        if (subdivisions == 0) {
            EterBase::ModernLogger::Error("PathSplineSmoother: Invalid subdivisions (0) requested for Entity {}.", entityId.value());
            return std::unexpected(SplineError::InvalidSubdivisions);
        }

        std::vector<Waypoint> smoothedPath;
        // Reserve approximate capacity
        smoothedPath.reserve(inputPath.size() * subdivisions);

        // Catmull-Rom requires 4 points for interpolation.
        // We handle endpoints by duplicating them.
        for (size_t i = 0; i < inputPath.size() - 1; ++i) {
            const Waypoint& p0 = (i == 0) ? inputPath[i] : inputPath[i - 1];
            const Waypoint& p1 = inputPath[i];
            const Waypoint& p2 = inputPath[i + 1];
            const Waypoint& p3 = (i + 2 < inputPath.size()) ? inputPath[i + 2] : inputPath[i + 1];

            // Avoid duplicate points at the exact same position if they occur.
            if (i > 0 && p1.x == p2.x && p1.y == p2.y && p1.z == p2.z) {
                continue;
            }

            for (uint32_t step = 0; step < subdivisions; ++step) {
                float t = static_cast<float>(step) / static_cast<float>(subdivisions);
                smoothedPath.push_back(CalculateCatmullRom(p0, p1, p2, p3, t));
            }
        }
        
        // Add the final point to ensure destination is exactly reached
        smoothedPath.push_back(inputPath.back());

        EterBase::ModernLogger::Debug(
            "PathSplineSmoother: Entity {} path smoothed. Original: {}, New: {}", 
            entityId.value(), inputPath.size(), smoothedPath.size()
        );

        // Publish event indicating a path was smoothed
        Core::EventBus::Instance().Publish(
            PathSmoothedEvent(entityId, inputPath.size(), smoothedPath.size())
        );

        return smoothedPath;
    }

private:
    /**
     * @brief Computes a point on a Catmull-Rom spline.
     * 
     * @param p0 The previous control point.
     * @param p1 The start control point.
     * @param p2 The end control point.
     * @param p3 The next control point.
     * @param t The interpolation parameter in range [0, 1).
     * @return Waypoint The interpolated point.
     */
    Waypoint CalculateCatmullRom(const Waypoint& p0, const Waypoint& p1, const Waypoint& p2, const Waypoint& p3, float t) const {
        float t2 = t * t;
        float t3 = t2 * t;

        float q0 = -t3 + 2.0f * t2 - t;
        float q1 = 3.0f * t3 - 5.0f * t2 + 2.0f;
        float q2 = -3.0f * t3 + 4.0f * t2 + t;
        float q3 = t3 - t2;

        Waypoint result;
        result.x = 0.5f * (p0.x * q0 + p1.x * q1 + p2.x * q2 + p3.x * q3);
        result.y = 0.5f * (p0.y * q0 + p1.y * q1 + p2.y * q2 + p3.y * q3);
        result.z = 0.5f * (p0.z * q0 + p1.z * q1 + p2.z * q2 + p3.z * q3);

        return result;
    }
};

} // namespace Navigation
