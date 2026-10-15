#pragma once

#include <cmath>
#include <span>
#include <expected>
#include <numeric>
#include <cstdint>

#include "PathNode.h"
#include "../EterBase/Result.h"
#include "../EterBase/StrongTypes.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Event published when the ETA for an entity is successfully calculated.
 */
struct EtaCalculatedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    float etaSeconds;

    /**
     * @brief Constructs an EtaCalculatedEvent.
     * @param id The entity ID.
     * @param eta The estimated time of arrival in seconds.
     */
    EtaCalculatedEvent(EterBase::EntityId id, float eta)
        : entityId(id), etaSeconds(eta) {}
};

/**
 * @brief A modern C++23 calculator for path lengths and arrival times.
 * 
 * Provides static methods to compute the total distance along a sequence of PathNodes
 * and to estimate the time it will take for an entity to traverse that path.
 */
class PathLengthCostCalculator {
public:
    PathLengthCostCalculator() = delete;

    /**
     * @brief Calculates the total geometric length of a given path.
     * 
     * Iterates over a span of path nodes and calculates the Euclidean distance 
     * between consecutive nodes.
     * 
     * @param path Span of PathNode structures representing the path.
     * @return std::expected containing the total float length on success, or a NavigationError on failure.
     */
    static std::expected<float, EterBase::NavigationError> CalculateTotalLength(std::span<const PathNode> path) {
        if (path.empty()) {
            EterBase::ModernLogger::Warn("CalculateTotalLength: Provided path is empty.");
            return std::unexpected(EterBase::NavigationError::PathNotFound);
        }

        if (path.size() == 1) {
            return 0.0f;
        }

        float totalLength = 0.0f;
        for (size_t i = 0; i < path.size() - 1; ++i) {
            float dx = static_cast<float>(path[i + 1].x - path[i].x);
            float dy = static_cast<float>(path[i + 1].y - path[i].y);
            totalLength += std::sqrt(dx * dx + dy * dy);
        }

        return totalLength;
    }

    /**
     * @brief Estimates the time of arrival for an entity traveling along a path.
     * 
     * Calculates the path length, computes ETA based on the given speed, and publishes an EtaCalculatedEvent.
     * 
     * @param entityId The unique identifier of the moving entity.
     * @param path The path the entity will travel.
     * @param speed The travel speed in units per second.
     * @return std::expected containing the calculated ETA in seconds, or a NavigationError on failure.
     */
    static std::expected<float, EterBase::NavigationError> EstimateTimeOfArrivalForEntity(
        EterBase::EntityId entityId, 
        std::span<const PathNode> path, 
        float speed) 
    {
        if (speed <= 0.0f) {
            EterBase::ModernLogger::Error("EstimateTimeOfArrivalForEntity: Speed must be greater than 0.");
            return std::unexpected(EterBase::NavigationError::DestinationUnreachable);
        }

        auto result = CalculateTotalLength(path)
            .transform([speed](float length) {
                return length / speed;
            });

        if (result.has_value()) {
            UserInterface::Core::EventBus::GetInstance().Publish(EtaCalculatedEvent{entityId, result.value()});
            EterBase::ModernLogger::Debug("Calculated ETA for Entity {}: {} seconds.", entityId.value(), result.value());
        }

        return result;
    }
};

} // namespace GameLib
