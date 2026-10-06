#pragma once

#include <vector>
#include <optional>
#include <format>

#include "../EterBase/StrongTypes.h"
#include "../EterBase/Result.h"
#include "../EterBase/LogModern.h"
namespace Core {
    class EventBus {
    public:
        static EventBus& Instance();
        template <typename T>
        void Publish(const T&);
    };
} // namespace Core

namespace GameLib {

/**
 * @brief Represents a 2D point used in navigation mesh polygons.
 */
struct NavPoint {
    float x{0.0f};
    float y{0.0f};

    /**
     * @brief Computes the cross product of two 2D vectors (this - a) and (b - a).
     * @param a The start point.
     * @param b The end point.
     * @return The 2D cross product scalar.
     */
    [[nodiscard]] constexpr float CrossProduct(const NavPoint& a, const NavPoint& b) const noexcept {
        return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
    }
};

/**
 * @brief Event emitted when an entity crosses or enters a navigation mesh polygon.
 */
struct NavMeshPolygonCrossedEvent {
    EterBase::EntityId entityId;
    uint32_t polygonId;

    /**
     * @brief Constructs the navigation mesh crossed event.
     * @param entityId Strong type ID of the entity.
     * @param polygonId Unique identifier of the polygon.
     */
    NavMeshPolygonCrossedEvent(EterBase::EntityId entityId, uint32_t polygonId)
        : entityId(entityId), polygonId(polygonId) {}
};

/**
 * @brief Lightweight representation of a convex navigation polygon.
 * 
 * This class handles pure data and spatial logic, strictly decoupling navigation 
 * from any graphical or rendering overhead (GUI).
 */
class NavMeshPolygonLite {
public:
    /**
     * @brief Constructs a new navigation polygon.
     * @param id Unique identifier for this polygon.
     */
    explicit NavMeshPolygonLite(uint32_t id) : id(id) {}

    /**
     * @brief Adds a vertex to the convex polygon.
     * @param point The 2D point to add.
     * @return A Result indicating success or an error if invalid.
     */
    EterBase::Result<void, EterBase::EntityError> AddVertex(const NavPoint& point) {
        vertices.push_back(point);
        EterBase::ModernLogger::Debug("Added vertex ({}, {}) to polygon {}", point.x, point.y, id);
        return {};
    }

    /**
     * @brief Validates if the current vertices form a valid polygon.
     * @return True if there are at least 3 vertices, otherwise false.
     */
    [[nodiscard]] bool IsValid() const noexcept {
        return vertices.size() >= 3;
    }

    /**
     * @brief Checks if a given point is strictly inside the convex polygon.
     * @param point The point to check.
     * @return std::optional<bool> containing true if inside, false if outside, and std::nullopt if the polygon is invalid.
     */
    [[nodiscard]] std::optional<bool> Contains(const NavPoint& point) const noexcept {
        if (!IsValid()) {
            return std::nullopt;
        }

        bool hasPositive = false;
        bool hasNegative = false;

        const size_t numVertices = vertices.size();
        for (size_t i = 0; i < numVertices; ++i) {
            const auto& a = vertices[i];
            const auto& b = vertices[(i + 1) % numVertices];

            float cross = point.CrossProduct(a, b);
            if (cross > 0.0f) {
                hasPositive = true;
            } else if (cross < 0.0f) {
                hasNegative = true;
            }

            if (hasPositive && hasNegative) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Processes an entity's movement and checks for polygon containment.
     *        Emits a NavMeshPolygonCrossedEvent if the entity enters the polygon.
     * @param entityId The strong domain ID of the entity.
     * @param currentPosition The current position of the entity.
     * @return Result on success, or an EntityError if invalid.
     */
    EterBase::Result<void, EterBase::EntityError> ProcessEntityMovement(EterBase::EntityId entityId, const NavPoint& currentPosition) {
        if (!entityId) {
            EterBase::ModernLogger::Error("ProcessEntityMovement called with invalid entity ID.");
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        auto containsResult = Contains(currentPosition);

        return containsResult
            .transform([this, entityId](bool isInside) -> EterBase::Result<void, EterBase::EntityError> {
                if (isInside) {
                    EterBase::ModernLogger::Info("Entity {} entered polygon {}", entityId.value(), id);
                    NavMeshPolygonCrossedEvent event{entityId, id};
                    Core::EventBus::Instance().Publish(event);
                }
                return {};
            })
            .value_or(EterBase::MakeError(EterBase::EntityError::InvalidType));
    }

private:
    uint32_t id;
    std::vector<NavPoint> vertices;
};

} // namespace GameLib
