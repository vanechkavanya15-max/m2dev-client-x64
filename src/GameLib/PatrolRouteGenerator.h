#pragma once

#include <vector>
#include <optional>
#include <cmath>
#include <numbers>
#include <expected>

#include "../EterBase/Result.h"
#include "../EterBase/StrongTypes.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"

namespace GameLib::Navigation {

/**
 * @brief Represents a 2D coordinate point on the map.
 */
struct Point2D {
    float x;
    float y;
};

/**
 * @brief Configuration for a patrol route.
 */
struct PatrolRouteConfig {
    Point2D center;       ///< The center point of the patrol loop
    float radius;         ///< The radius of the patrol loop
    size_t pointCount;    ///< The number of points in the patrol loop
};

/**
 * @brief Event emitted when a patrol route is successfully generated.
 */
struct PatrolRouteGeneratedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    std::vector<Point2D> route;

    /**
     * @brief Constructs the event with entity and route data.
     * @param id The entity identifier.
     * @param generatedRoute The generated patrol points.
     */
    PatrolRouteGeneratedEvent(EterBase::EntityId id, std::vector<Point2D> generatedRoute)
        : entityId(id), route(std::move(generatedRoute)) {}
};

/**
 * @brief Generator for entity patrol routes.
 * 
 * Uses modern C++23 features like std::expected, std::optional, and std::format
 * to provide a safe and robust navigation pathing system.
 */
class PatrolRouteGenerator {
public:
    /**
     * @brief Generates a circular patrol route around a specified center point.
     * 
     * @param entityId The unique identifier of the entity.
     * @param config The configuration containing center, radius, and point count.
     * @return std::expected<std::vector<Point2D>, EterBase::NavigationError> containing the route or an error.
     */
    static std::expected<std::vector<Point2D>, EterBase::NavigationError> GenerateRoute(
        EterBase::EntityId entityId, const PatrolRouteConfig& config) 
    {
        if (config.pointCount < 3) {
            EterBase::ModernLogger::Error("Failed to generate route for entity {}: pointCount too small", entityId);
            return std::unexpected(EterBase::NavigationError::PathNotFound);
        }

        if (config.radius <= 0.0f) {
            EterBase::ModernLogger::Error("Failed to generate route for entity {}: invalid radius", entityId);
            return std::unexpected(EterBase::NavigationError::DestinationUnreachable);
        }

        std::vector<Point2D> route;
        route.reserve(config.pointCount);

        const float angleStep = 2.0f * std::numbers::pi_v<float> / static_cast<float>(config.pointCount);

        for (size_t i = 0; i < config.pointCount; ++i) {
            const float currentAngle = angleStep * static_cast<float>(i);
            route.push_back({
                config.center.x + config.radius * std::cos(currentAngle),
                config.center.y + config.radius * std::sin(currentAngle)
            });
        }

        EterBase::ModernLogger::Info("Successfully generated route for entity {} with {} points", entityId, config.pointCount);

        // Publish event to decouple from GUI
        UserInterface::Core::EventBus::GetInstance().Publish(PatrolRouteGeneratedEvent{entityId, route});

        return route;
    }

    /**
     * @brief Adjusts an existing route to avoid a given obstacle.
     * 
     * Demonstrates C++23 monadic operations with std::optional.
     * 
     * @param originalRoute The original route to adjust. Passed as r-value reference to avoid double-copying.
     * @param obstacleCenter The center of the obstacle.
     * @param obstacleRadius The radius of the obstacle.
     * @return std::optional<std::vector<Point2D>> The adjusted route, or std::nullopt if the original route is empty.
     */
    static std::optional<std::vector<Point2D>> AdjustRouteForObstacle(
        std::optional<std::vector<Point2D>>&& originalRoute, 
        Point2D obstacleCenter, 
        float obstacleRadius) 
    {
        return std::move(originalRoute).transform([&](std::vector<Point2D> route) {
            for (auto& point : route) {
                float dx = point.x - obstacleCenter.x;
                float dy = point.y - obstacleCenter.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                
                if (dist < obstacleRadius && dist > 0.0f) {
                    // Push point outward
                    float pushFactor = (obstacleRadius / dist);
                    point.x = obstacleCenter.x + dx * pushFactor;
                    point.y = obstacleCenter.y + dy * pushFactor;
                }
            }
            return route;
        });
    }
};

} // namespace GameLib::Navigation
