#include <cstdint>
#include <vector>
#include <queue>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace GameLib {

/**
 * @brief Represents a 2D coordinate in the grid.
 */
struct Point {
    uint32_t x;
    uint32_t y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

} // namespace GameLib

namespace std {
    template <>
    struct hash<GameLib::Point> {
        size_t operator()(const GameLib::Point& p) const {
            return (static_cast<size_t>(p.x) << 32) | p.y;
        }
    };
}

namespace GameLib {

/**
 * @brief Interface for a grid providing collision data.
 */
class CollisionGrid {
public:
    virtual ~CollisionGrid() = default;

    /**
     * @brief Checks if a given point in the grid is an obstacle.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @return true if the cell is blocked, false if it is walkable.
     */
    virtual bool IsBlocked(uint32_t x, uint32_t y) const = 0;

    /**
     * @brief Gets the width of the grid.
     * @return The width in cells.
     */
    virtual uint32_t GetWidth() const = 0;

    /**
     * @brief Gets the height of the grid.
     * @return The height in cells.
     */
    virtual uint32_t GetHeight() const = 0;
};

/**
 * @brief Navigator class using A* algorithm to find paths on a CollisionGrid.
 */
class AStarNavigator {
public:
    AStarNavigator() = default;
    ~AStarNavigator() = default;

    /**
     * @brief Finds a path from start to end using A*.
     * @param grid The collision grid to navigate on.
     * @param start The starting coordinate.
     * @param end The ending coordinate.
     * @return A vector of points representing the path from start to end. Empty if no path found.
     */
    std::vector<Point> FindPath(const CollisionGrid& grid, const Point& start, const Point& end) const {
        if (start.x >= grid.GetWidth() || start.y >= grid.GetHeight() ||
            end.x >= grid.GetWidth() || end.y >= grid.GetHeight()) {
            return {};
        }

        if (grid.IsBlocked(start.x, start.y) || grid.IsBlocked(end.x, end.y)) {
            return {};
        }

        if (start == end) {
            return {start};
        }

        struct Node {
            Point position;
            uint32_t gCost;
            uint32_t fCost;

            bool operator>(const Node& other) const {
                return fCost > other.fCost;
            }
        };

        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
        std::unordered_map<Point, Point> cameFrom;
        std::unordered_map<Point, uint32_t> gScore;

        openSet.push({start, 0, Heuristic(start, end)});
        gScore[start] = 0;

        const int32_t directions[8][2] = {
            {0, 1}, {1, 0}, {0, -1}, {-1, 0},
            {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
        };

        while (!openSet.empty()) {
            Node current = openSet.top();
            openSet.pop();

            if (current.position == end) {
                return ReconstructPath(cameFrom, current.position);
            }

            // If we found a better path earlier, skip this node
            if (current.gCost > gScore[current.position]) {
                continue;
            }

            for (const auto& dir : directions) {
                int32_t nx = static_cast<int32_t>(current.position.x) + dir[0];
                int32_t ny = static_cast<int32_t>(current.position.y) + dir[1];

                if (nx < 0 || ny < 0) {
                    continue;
                }

                uint32_t unx = static_cast<uint32_t>(nx);
                uint32_t uny = static_cast<uint32_t>(ny);

                if (unx >= grid.GetWidth() || uny >= grid.GetHeight() || grid.IsBlocked(unx, uny)) {
                    continue;
                }

                Point neighbor = {unx, uny};
                uint32_t tentativeGScore = current.gCost + ((dir[0] == 0 || dir[1] == 0) ? 10 : 14);

                auto it = gScore.find(neighbor);
                if (it == gScore.end() || tentativeGScore < it->second) {
                    cameFrom[neighbor] = current.position;
                    gScore[neighbor] = tentativeGScore;
                    openSet.push({neighbor, tentativeGScore, tentativeGScore + Heuristic(neighbor, end)});
                }
            }
        }

        return {};
    }

private:
    /**
     * @brief Calculates the heuristic cost between two points.
     * @param a The first point.
     * @param b The second point.
     * @return The heuristic cost.
     */
    uint32_t Heuristic(const Point& a, const Point& b) const {
        uint32_t dx = (a.x > b.x) ? (a.x - b.x) : (b.x - a.x);
        uint32_t dy = (a.y > b.y) ? (a.y - b.y) : (b.y - a.y);
        
        // Diagonal distance heuristic
        if (dx > dy) {
            return 14 * dy + 10 * (dx - dy);
        } else {
            return 14 * dx + 10 * (dy - dx);
        }
    }

    /**
     * @brief Reconstructs the path from the cameFrom map.
     * @param cameFrom The map of nodes to their parent nodes.
     * @param current The current end node.
     * @return The reconstructed path.
     */
    std::vector<Point> ReconstructPath(const std::unordered_map<Point, Point>& cameFrom, Point current) const {
        std::vector<Point> path;
        path.push_back(current);
        auto it = cameFrom.find(current);
        while (it != cameFrom.end()) {
            current = it->second;
            path.push_back(current);
            it = cameFrom.find(current);
        }
        std::reverse(path.begin(), path.end());
        return path;
    }
};

} // namespace GameLib
