#pragma once

#include <vector>
#include <optional>
#include <cstdint>
#include "../EterBase/Result.h"
#include "../EterBase/StrongTypes.h"
#include "CollisionGrid.h"

namespace GameLib {

/**
 * @brief Represents a 2D coordinate in the navigation grid.
 */
struct Point {
    uint32_t x;
    uint32_t y;

    constexpr bool operator==(const Point& other) const noexcept {
        return x == other.x && y == other.y;
    }
};

/**
 * @brief Ultra-fast Jump Point Search (JPS) pathfinding algorithm navigator.
 * 
 * Computes paths on a CollisionGrid significantly faster than traditional A* 
 * by skipping symmetrically redundant nodes (jump points).
 */
class JumpPointSearchNavigator {
public:
    /**
     * @brief Constructs a new Jump Point Search Navigator.
     */
    JumpPointSearchNavigator() = default;

    /**
     * @brief Destroys the Jump Point Search Navigator.
     */
    ~JumpPointSearchNavigator() = default;

    /**
     * @brief Finds a path from the start point to the end point on the given grid.
     * 
     * @param grid The collision grid representing walkable and non-walkable areas.
     * @param start The starting coordinate.
     * @param end The destination coordinate.
     * @param entityId The ID of the entity requesting the path (used for event broadcasting).
     * @return EterBase::Result<std::vector<Point>, EterBase::NavigationError> The calculated path, or an error if unreachable.
     */
    EterBase::Result<std::vector<Point>, EterBase::NavigationError> FindPath(
        const CollisionGrid& grid, 
        const Point& start, 
        const Point& end,
        EterBase::EntityId entityId) const;

private:
    /**
     * @brief Recursively searches in a direction for a jump point.
     * 
     * @param grid The collision grid.
     * @param current The current coordinate being evaluated.
     * @param dx The direction of travel on the x-axis (-1, 0, 1).
     * @param dy The direction of travel on the y-axis (-1, 0, 1).
     * @param end The destination coordinate.
     * @return std::optional<Point> The found jump point, or nullopt if none found.
     */
    std::optional<Point> Jump(const CollisionGrid& grid, const Point& current, int dx, int dy, const Point& end) const;

    /**
     * @brief Evaluates whether a specific coordinate is walkable.
     * 
     * @param grid The collision grid.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @return true If the cell is within bounds and not an obstacle.
     * @return false Otherwise.
     */
    bool IsWalkable(const CollisionGrid& grid, int x, int y) const noexcept;
};

} // namespace GameLib

namespace std {
    /**
     * @brief Hash specialization for GameLib::Point for use in unordered associative containers.
     */
    template <>
    struct hash<GameLib::Point> {
        size_t operator()(const GameLib::Point& p) const noexcept {
            return (static_cast<size_t>(p.x) << 32) | p.y;
        }
    };
}
