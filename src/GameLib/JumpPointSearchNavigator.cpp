#include "JumpPointSearchNavigator.h"
#include "../UserInterface/Core/EventBus.h"
#include "../EterBase/LogModern.h"

#include <queue>
#include <unordered_map>
#include <cmath>
#include <algorithm>

namespace GameLib {

// Event emitted when a path is successfully found
struct PathFoundEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    std::vector<Point> path;

    PathFoundEvent(EterBase::EntityId entityId, std::vector<Point> path)
        : entityId(entityId), path(std::move(path)) {}
};

bool JumpPointSearchNavigator::IsWalkable(const CollisionGrid& grid, int x, int y) const noexcept {
    if (x < 0 || y < 0) {
        return false;
    }
    
    // grid.IsObstacle expects uint32_t. It safely handles out of bounds.
    return !grid.IsObstacle(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
}

std::optional<Point> JumpPointSearchNavigator::Jump(
    const CollisionGrid& grid, 
    const Point& current, 
    int dx, 
    int dy, 
    const Point& end) const {
        
    int nx = static_cast<int>(current.x) + dx;
    int ny = static_cast<int>(current.y) + dy;
    
    if (!IsWalkable(grid, nx, ny)) {
        return std::nullopt;
    }
    
    Point nextPoint{static_cast<uint32_t>(nx), static_cast<uint32_t>(ny)};
    
    if (nextPoint == end) {
        return nextPoint;
    }
    
    // Diagonal jump
    if (dx != 0 && dy != 0) {
        // Check for forced neighbors
        if ((IsWalkable(grid, nx - dx, ny + dy) && !IsWalkable(grid, nx - dx, ny)) ||
            (IsWalkable(grid, nx + dx, ny - dy) && !IsWalkable(grid, nx, ny - dy))) {
            return nextPoint;
        }
        
        // When moving diagonally, must check for vertical/horizontal jump points
        if (Jump(grid, nextPoint, dx, 0, end) || Jump(grid, nextPoint, 0, dy, end)) {
            return nextPoint;
        }
    } 
    // Horizontal/Vertical jump
    else {
        if (dx != 0) {
            if ((IsWalkable(grid, nx + dx, ny + 1) && !IsWalkable(grid, nx, ny + 1)) ||
                (IsWalkable(grid, nx + dx, ny - 1) && !IsWalkable(grid, nx, ny - 1))) {
                return nextPoint;
            }
        } else {
            if ((IsWalkable(grid, nx + 1, ny + dy) && !IsWalkable(grid, nx + 1, ny)) ||
                (IsWalkable(grid, nx - 1, ny + dy) && !IsWalkable(grid, nx - 1, ny))) {
                return nextPoint;
            }
        }
    }
    
    return Jump(grid, nextPoint, dx, dy, end);
}

EterBase::Result<std::vector<Point>, EterBase::NavigationError> JumpPointSearchNavigator::FindPath(
    const CollisionGrid& grid, 
    const Point& start, 
    const Point& end,
    EterBase::EntityId entityId) const {

    if (grid.IsObstacle(start.x, start.y) || grid.IsObstacle(end.x, end.y)) {
        EterBase::ModernLogger::Warn("Path search failed: start ({}, {}) or end ({}, {}) is blocked.", start.x, start.y, end.x, end.y);
        return std::unexpected(EterBase::NavigationError::BlockedTerrain);
    }

    if (start == end) {
        return std::vector<Point>{start};
    }

    struct Node {
        Point p;
        double gCost;
        double fCost;

        bool operator>(const Node& other) const {
            return fCost > other.fCost;
        }
    };

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
    std::unordered_map<Point, Point> cameFrom;
    std::unordered_map<Point, double> gScore;

    auto calculateHeuristic = [](const Point& a, const Point& b) {
        double dx = std::abs(static_cast<double>(a.x) - static_cast<double>(b.x));
        double dy = std::abs(static_cast<double>(a.y) - static_cast<double>(b.y));
        return std::max(dx, dy) + (std::sqrt(2.0) - 1.0) * std::min(dx, dy); // Octile heuristic
    };

    openSet.push(Node{start, 0.0, calculateHeuristic(start, end)});
    gScore[start] = 0.0;

    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();

        if (current.p == end) {
            std::vector<Point> path;
            Point curr = end;
            
            while (!(curr == start)) {
                path.push_back(curr);
                curr = cameFrom[curr];
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
            
            // Emit successful path event
            UserInterface::Core::EventBus::GetInstance().Publish(PathFoundEvent(entityId, path));
            EterBase::ModernLogger::Debug("JumpPointSearchNavigator: Path found for Entity {}, length {}", entityId.get(), path.size());
            
            return path;
        }

        if (current.gCost > gScore[current.p]) {
            continue;
        }

        std::vector<std::pair<int, int>> successors;
        
        if (current.p == start) {
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx != 0 || dy != 0) {
                        successors.push_back({dx, dy});
                    }
                }
            }
        } else {
            Point parent = cameFrom[current.p];
            int dx = (current.p.x > parent.x) ? 1 : ((current.p.x < parent.x) ? -1 : 0);
            int dy = (current.p.y > parent.y) ? 1 : ((current.p.y < parent.y) ? -1 : 0);

            if (dx != 0 && dy != 0) {
                successors.push_back({dx, dy});
                successors.push_back({dx, 0});
                successors.push_back({0, dy});
                
                if (!IsWalkable(grid, static_cast<int>(current.p.x) - dx, static_cast<int>(current.p.y))) {
                    successors.push_back({-dx, dy});
                }
                if (!IsWalkable(grid, static_cast<int>(current.p.x), static_cast<int>(current.p.y) - dy)) {
                    successors.push_back({dx, -dy});
                }
            } else {
                if (dx != 0) {
                    successors.push_back({dx, 0});
                    if (!IsWalkable(grid, static_cast<int>(current.p.x), static_cast<int>(current.p.y) + 1)) {
                        successors.push_back({dx, 1});
                    }
                    if (!IsWalkable(grid, static_cast<int>(current.p.x), static_cast<int>(current.p.y) - 1)) {
                        successors.push_back({dx, -1});
                    }
                } else {
                    successors.push_back({0, dy});
                    if (!IsWalkable(grid, static_cast<int>(current.p.x) + 1, static_cast<int>(current.p.y))) {
                        successors.push_back({1, dy});
                    }
                    if (!IsWalkable(grid, static_cast<int>(current.p.x) - 1, static_cast<int>(current.p.y))) {
                        successors.push_back({-1, dy});
                    }
                }
            }
        }

        for (const auto& [dx, dy] : successors) {
            std::optional<Point> jumpPoint = Jump(grid, current.p, dx, dy, end);
            
            if (jumpPoint.has_value()) {
                double distance = calculateHeuristic(current.p, jumpPoint.value());
                double tentativeGScore = current.gCost + distance;
                
                auto it = gScore.find(jumpPoint.value());
                if (it == gScore.end() || tentativeGScore < it->second) {
                    gScore[jumpPoint.value()] = tentativeGScore;
                    cameFrom[jumpPoint.value()] = current.p;
                    double fCost = tentativeGScore + calculateHeuristic(jumpPoint.value(), end);
                    openSet.push(Node{jumpPoint.value(), tentativeGScore, fCost});
                }
            }
        }
    }

    EterBase::ModernLogger::Warn("JumpPointSearchNavigator: Path not found from ({}, {}) to ({}, {}) for Entity {}", start.x, start.y, end.x, end.y, entityId.get());
    return std::unexpected(EterBase::NavigationError::PathNotFound);
}

} // namespace GameLib
