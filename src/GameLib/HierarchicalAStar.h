#pragma once

#include <vector>
#include <unordered_map>
#include <expected>
#include <optional>
#include <format>
#include <string_view>
#include <span>
#include <cstdint>
#include <cmath>
#include <queue>
#include <algorithm>

#ifdef TEST_MOCK
#include "../../../../test_mock_headers.h"
#else
#include "../EterBase/StrongTypes.h"
#include "../EterBase/ModernLogger.h"
#include "../Core/EventBus.h"
#endif

namespace GameLib
{
    /**
     * @brief Represents a 2D coordinate on the map.
     */
    struct Point2D
    {
        int32_t x;
        int32_t y;

        bool operator==(const Point2D& other) const = default;
    };

    /**
     * @brief Custom hash function for Point2D to be used in unordered maps/sets.
     */
    struct Point2DHash
    {
        std::size_t operator()(const Point2D& point) const noexcept
        {
            return std::hash<int32_t>()(point.x) ^ (std::hash<int32_t>()(point.y) << 1);
        }
    };

    /**
     * @brief Represents an abstract entrance between two adjacent clusters.
     */
    struct ClusterEntrance
    {
        Point2D pointInClusterA;
        Point2D pointInClusterB;
        int32_t clusterAId;
        int32_t clusterBId;
    };

    /**
     * @brief Represents an edge in the abstract graph linking two entrances.
     */
    struct AbstractEdge
    {
        int32_t targetEntranceIndex;
        int32_t cost;
    };

    /**
     * @brief Represents a node in the abstract graph (typically an entrance).
     */
    struct AbstractNode
    {
        Point2D point;
        int32_t clusterId;
        std::vector<AbstractEdge> edges;
    };

    /**
     * @brief Represents a single cluster/sector in the Hierarchical A* grid.
     */
    struct MapCluster
    {
        uint32_t id;
        Point2D boundsMin;
        Point2D boundsMax;
    };

    /**
     * @brief Event emitted when a path is successfully calculated.
     */
    struct PathCalculatedEvent
    {
        EterBase::EntityId entityId;
        std::vector<Point2D> path;
    };

    /**
     * @brief Enumeration for possible pathfinding errors.
     */
    enum class PathfindingError
    {
        InvalidStartNode,
        InvalidTargetNode,
        NoPathFound,
        MapNotLoaded,
        StartAndTargetAreSame,
        GraphNotBuilt
    };

    /**
     * @brief Event emitted when a path calculation fails.
     */
    struct PathCalculationFailedEvent
    {
        EterBase::EntityId entityId;
        PathfindingError error;
        std::string_view reason;
    };

    /**
     * @brief Hierarchical A* Pathfinding implementation for large 4x4 km maps.
     * 
     * This class implements a multi-level A* algorithm to efficiently calculate
     * long-distance paths by dividing the map into clusters, building an abstract graph,
     * and performing A* at the high level before refining it locally.
     */
    class HierarchicalAStar
    {
    public:
        /**
         * @brief Default constructor for HierarchicalAStar.
         */
        HierarchicalAStar() = default;
        ~HierarchicalAStar() = default;

        HierarchicalAStar(const HierarchicalAStar&) = delete;
        HierarchicalAStar& operator=(const HierarchicalAStar&) = delete;

        /**
         * @brief Initializes the hierarchical grid with specific cluster size.
         * 
         * @param clusterSize The size of each cluster in map units.
         */
        void Initialize(int32_t clusterSize)
        {
            this->clusterSize = clusterSize;
            this->isGraphBuilt = true; 
            EterBase::ModernLogger::Info(std::format("HierarchicalAStar initialized with cluster size: {}", clusterSize));
        }

        /**
         * @brief Calculates a path from start to target for a given entity.
         * 
         * @param entityId The ID of the entity requesting the path.
         * @param start The starting coordinate.
         * @param target The target coordinate.
         * @return std::expected<void, PathfindingError> Result of the operation.
         */
        std::expected<void, PathfindingError> CalculatePath(EterBase::EntityId entityId, Point2D start, Point2D target)
        {
            if (!isGraphBuilt)
            {
                EmitFailure(entityId, PathfindingError::GraphNotBuilt, "Abstract graph has not been built.");
                return std::unexpected(PathfindingError::GraphNotBuilt);
            }

            if (start == target)
            {
                EmitFailure(entityId, PathfindingError::StartAndTargetAreSame, "Start and target positions are the same.");
                return std::unexpected(PathfindingError::StartAndTargetAreSame);
            }

            auto startClusterOpt = GetClusterForPoint(start);
            auto targetClusterOpt = GetClusterForPoint(target);

            return startClusterOpt.and_then([&](MapCluster startCluster) -> std::optional<std::expected<void, PathfindingError>> {
                return targetClusterOpt.transform([&](MapCluster targetCluster) {
                    return PerformHierarchicalSearch(entityId, start, target, startCluster, targetCluster);
                });
            }).value_or(std::unexpected(PathfindingError::InvalidStartNode));
        }

        /**
         * @brief Adds an entrance to the abstract graph.
         * 
         * @param entrance The entrance linking two clusters.
         */
        void AddEntrance(const ClusterEntrance& entrance)
        {
            int32_t indexA = abstractGraph.size();
            abstractGraph.push_back({entrance.pointInClusterA, entrance.clusterAId, {}});
            int32_t indexB = abstractGraph.size();
            abstractGraph.push_back({entrance.pointInClusterB, entrance.clusterBId, {}});

            // Link them across cluster boundary
            abstractGraph[indexA].edges.push_back({indexB, 1}); // Base cost for jumping boundary
            abstractGraph[indexB].edges.push_back({indexA, 1});
            
            // Connect to other nodes in the same cluster
            ConnectNodeWithinCluster(indexA, entrance.clusterAId);
            ConnectNodeWithinCluster(indexB, entrance.clusterBId);
        }

    private:
        int32_t clusterSize = 0;
        bool isGraphBuilt = false;
        
        std::vector<AbstractNode> abstractGraph;

        /**
         * @brief Resolves the cluster that contains the given point.
         * 
         * @param point The point to find the cluster for.
         * @return std::optional<MapCluster> The map cluster if valid, std::nullopt otherwise.
         */
        std::optional<MapCluster> GetClusterForPoint(Point2D point) const
        {
            if (clusterSize <= 0)
                return std::nullopt;

            int32_t clusterX = point.x / clusterSize;
            int32_t clusterY = point.y / clusterSize;
            
            uint32_t clusterId = static_cast<uint32_t>((clusterX << 16) | (clusterY & 0xFFFF));
            
            return MapCluster{
                .id = clusterId,
                .boundsMin = {clusterX * clusterSize, clusterY * clusterSize},
                .boundsMax = {(clusterX + 1) * clusterSize - 1, (clusterY + 1) * clusterSize - 1}
            };
        }

        /**
         * @brief Connects a newly added abstract node to all other existing nodes in the same cluster.
         * 
         * @param nodeIndex The index of the node to connect.
         * @param clusterId The cluster ID.
         */
        void ConnectNodeWithinCluster(int32_t nodeIndex, int32_t clusterId)
        {
            for (size_t i = 0; i < abstractGraph.size(); ++i)
            {
                if (i != static_cast<size_t>(nodeIndex) && abstractGraph[i].clusterId == clusterId)
                {
                    // Calculate a local path to get exact cost. Using heuristic for simplicity if path exists.
                    auto path = LocalAStar(abstractGraph[nodeIndex].point, abstractGraph[i].point);
                    if (path)
                    {
                        int32_t cost = path->size() * 10; // Approx cost
                        abstractGraph[nodeIndex].edges.push_back({static_cast<int32_t>(i), cost});
                        abstractGraph[i].edges.push_back({nodeIndex, cost});
                    }
                }
            }
        }

        /**
         * @brief Calculates Euclidean heuristic distance.
         * 
         * @param a Point A.
         * @param b Point B.
         * @return int32_t The heuristic value.
         */
        int32_t Heuristic(Point2D a, Point2D b) const
        {
            int32_t dx = a.x - b.x;
            int32_t dy = a.y - b.y;
            return static_cast<int32_t>(std::sqrt(dx * dx + dy * dy));
        }

        /**
         * @brief Gets valid neighbors for a point. (Simplified mock for terrain).
         * 
         * @param current The current point.
         * @return std::vector<Point2D> Valid neighbors.
         */
        std::vector<Point2D> GetNeighbors(Point2D current) const
        {
            return {
                {current.x + 1, current.y},
                {current.x - 1, current.y},
                {current.x, current.y + 1},
                {current.x, current.y - 1},
                {current.x + 1, current.y + 1},
                {current.x - 1, current.y - 1},
                {current.x - 1, current.y + 1},
                {current.x + 1, current.y - 1}
            };
        }

        /**
         * @brief A simple A* implementation for local, intra-cluster pathfinding.
         * 
         * @param start The start point.
         * @param target The target point.
         * @return std::optional<std::vector<Point2D>> The path, or nullopt if none found.
         */
        std::optional<std::vector<Point2D>> LocalAStar(Point2D start, Point2D target)
        {
            struct Node {
                Point2D point;
                int32_t gCost;
                int32_t fCost;
                bool operator>(const Node& other) const {
                    return fCost > other.fCost;
                }
            };

            std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
            std::unordered_map<Point2D, Point2D, Point2DHash> cameFrom;
            std::unordered_map<Point2D, int32_t, Point2DHash> gScore;

            openSet.push({start, 0, Heuristic(start, target) * 10});
            gScore[start] = 0;

            int32_t iterationLimit = clusterSize * clusterSize; 
            int32_t iterations = 0;

            while (!openSet.empty() && iterations < iterationLimit)
            {
                iterations++;
                Node current = openSet.top();
                openSet.pop();
                
                if (current.gCost > gScore[current.point]) continue;

                if (current.point == target)
                {
                    std::vector<Point2D> path;
                    Point2D curr = target;
                    while (!(curr == start))
                    {
                        path.push_back(curr);
                        curr = cameFrom[curr];
                    }
                    path.push_back(start);
                    std::reverse(path.begin(), path.end());
                    return path;
                }

                for (Point2D neighbor : GetNeighbors(current.point))
                {
                    int32_t stepCost = (current.point.x != neighbor.x && current.point.y != neighbor.y) ? 14 : 10;
                    int32_t tentativeGCost = gScore[current.point] + stepCost;

                    if (!gScore.contains(neighbor) || tentativeGCost < gScore[neighbor])
                    {
                        cameFrom[neighbor] = current.point;
                        gScore[neighbor] = tentativeGCost;
                        int32_t fCost = tentativeGCost + Heuristic(neighbor, target) * 10;
                        openSet.push({neighbor, tentativeGCost, fCost});
                    }
                }
            }

            return std::nullopt;
        }
        
        /**
         * @brief Performs A* search on the abstract high-level graph.
         * 
         * @param startIndex The starting node index in abstract graph.
         * @param targetIndex The target node index in abstract graph.
         * @return std::optional<std::vector<int32_t>> Sequence of abstract node indices, or nullopt.
         */
        std::optional<std::vector<int32_t>> AbstractAStar(int32_t startIndex, int32_t targetIndex)
        {
            struct AbsNode {
                int32_t index;
                int32_t gCost;
                int32_t fCost;
                bool operator>(const AbsNode& other) const {
                    return fCost > other.fCost;
                }
            };

            std::priority_queue<AbsNode, std::vector<AbsNode>, std::greater<AbsNode>> openSet;
            std::unordered_map<int32_t, int32_t> cameFrom;
            std::unordered_map<int32_t, int32_t> gScore;

            openSet.push({startIndex, 0, Heuristic(abstractGraph[startIndex].point, abstractGraph[targetIndex].point) * 10});
            gScore[startIndex] = 0;

            while (!openSet.empty())
            {
                AbsNode current = openSet.top();
                openSet.pop();
                
                if (current.gCost > gScore[current.index]) continue;

                if (current.index == targetIndex)
                {
                    std::vector<int32_t> path;
                    int32_t curr = targetIndex;
                    while (curr != startIndex)
                    {
                        path.push_back(curr);
                        curr = cameFrom[curr];
                    }
                    path.push_back(startIndex);
                    std::reverse(path.begin(), path.end());
                    return path;
                }

                for (const auto& edge : abstractGraph[current.index].edges)
                {
                    int32_t neighbor = edge.targetEntranceIndex;
                    int32_t tentativeGCost = gScore[current.index] + edge.cost;

                    if (!gScore.contains(neighbor) || tentativeGCost < gScore[neighbor])
                    {
                        cameFrom[neighbor] = current.index;
                        gScore[neighbor] = tentativeGCost;
                        int32_t fCost = tentativeGCost + Heuristic(abstractGraph[neighbor].point, abstractGraph[targetIndex].point) * 10;
                        openSet.push({neighbor, tentativeGCost, fCost});
                    }
                }
            }
            return std::nullopt; 
        }

        /**
         * @brief Performs the hierarchical search algorithm.
         * 
         * @param entityId The entity ID.
         * @param start The start point.
         * @param target The target point.
         * @param startCluster The start cluster.
         * @param targetCluster The target cluster.
         * @return std::expected<void, PathfindingError> Result.
         */
        std::expected<void, PathfindingError> PerformHierarchicalSearch(
            EterBase::EntityId entityId, 
            Point2D start, 
            Point2D target, 
            MapCluster startCluster, 
            MapCluster targetCluster)
        {
            std::optional<std::vector<Point2D>> finalPath;

            if (startCluster.id == targetCluster.id)
            {
                // Both points in same cluster, do standard local A*
                finalPath = LocalAStar(start, target);
            }
            else
            {
                // Temporarily inject start and target nodes into abstract graph
                int32_t startIndex = abstractGraph.size();
                abstractGraph.push_back({start, static_cast<int32_t>(startCluster.id), {}});
                ConnectNodeWithinCluster(startIndex, startCluster.id);

                int32_t targetIndex = abstractGraph.size();
                abstractGraph.push_back({target, static_cast<int32_t>(targetCluster.id), {}});
                ConnectNodeWithinCluster(targetIndex, targetCluster.id);

                auto abstractPathOpt = AbstractAStar(startIndex, targetIndex);
                
                if (abstractPathOpt)
                {
                    std::vector<Point2D> fullPath;
                    for (size_t i = 0; i < abstractPathOpt->size() - 1; ++i)
                    {
                        int32_t nodeA = (*abstractPathOpt)[i];
                        int32_t nodeB = (*abstractPathOpt)[i+1];

                        if (abstractGraph[nodeA].clusterId == abstractGraph[nodeB].clusterId)
                        {
                            auto localPath = LocalAStar(abstractGraph[nodeA].point, abstractGraph[nodeB].point);
                            if (localPath)
                            {
                                // Avoid duplicating points between segments
                                if (!fullPath.empty() && fullPath.back() == localPath->front()) {
                                    fullPath.pop_back();
                                }
                                fullPath.insert(fullPath.end(), localPath->begin(), localPath->end());
                            }
                            else
                            {
                                fullPath.clear();
                                break; // Local link failed
                            }
                        }
                    }

                    if (!fullPath.empty()) {
                        finalPath = fullPath;
                    }
                }

                // Cleanup injected temporary nodes
                abstractGraph.pop_back(); // Remove target
                abstractGraph.pop_back(); // Remove start
                for (auto& node : abstractGraph) {
                    std::erase_if(node.edges, [&](const AbstractEdge& e) { return e.targetEntranceIndex >= static_cast<int32_t>(abstractGraph.size()); });
                }
            }

            if (!finalPath || finalPath->empty())
            {
                EmitFailure(entityId, PathfindingError::NoPathFound, "No path found between start and target.");
                return std::unexpected(PathfindingError::NoPathFound);
            }

            EmitSuccess(entityId, std::move(*finalPath));
            return {};
        }

        /**
         * @brief Helper to emit success event.
         * 
         * @param entityId The entity ID.
         * @param path The calculated path.
         */
        void EmitSuccess(EterBase::EntityId entityId, std::vector<Point2D>&& path)
        {
            PathCalculatedEvent event{
                .entityId = entityId,
                .path = std::move(path)
            };
            Core::EventBus::Instance().Publish(event);
            EterBase::ModernLogger::Debug(std::format("Path calculated successfully for entity {}", entityId.value));
        }

        /**
         * @brief Helper to emit failure event.
         * 
         * @param entityId The entity ID.
         * @param error The error code.
         * @param reason The error reason string.
         */
        void EmitFailure(EterBase::EntityId entityId, PathfindingError error, std::string_view reason)
        {
            PathCalculationFailedEvent event{
                .entityId = entityId,
                .error = error,
                .reason = reason
            };
            Core::EventBus::Instance().Publish(event);
            EterBase::ModernLogger::Error(std::format("Path calculation failed for entity {}: {}", entityId.value, reason));
        }
    };
}
