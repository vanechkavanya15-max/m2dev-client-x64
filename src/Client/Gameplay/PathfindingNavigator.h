#pragma once

#include <vector>
#include <expected>
#include <optional>
#include <cmath>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include "Client/World/TerrainHeightSampler.h"

namespace Client::Gameplay {

    enum class PathfindingError {
        StartOutOfBounds,
        EndOutOfBounds,
        NoPathFound,
        TerrainNotLoaded
    };

    struct PathNode {
        Client::World::Vector3 position;
        float gCost; // Koszt od startu
        float hCost; // Heurystyka do celu

        float GetFCost() const {
            return gCost + hCost;
        }

        bool operator>(const PathNode& other) const {
            return GetFCost() > other.GetFCost();
        }
    };

    struct GridPoint {
        int x;
        int y;

        bool operator==(const GridPoint& other) const {
            return x == other.x && y == other.y;
        }
    };

} // namespace Client::Gameplay

template <>
struct std::hash<Client::Gameplay::GridPoint> {
    std::size_t operator()(const Client::Gameplay::GridPoint& p) const noexcept {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};

namespace Client::Gameplay {

    class PathfindingNavigator {
    public:
        PathfindingNavigator() = default;
        ~PathfindingNavigator() = default;

        /**
         * Znajduje trase omijajaca przeszkody na podstawie wysokosci terenu.
         * @param startX Wspolrzedna X punktu startowego.
         * @param startY Wspolrzedna Y punktu startowego.
         * @param endX Wspolrzedna X celu.
         * @param endY Wspolrzedna Y celu.
         * @param sampler Referencja do samplera terenu.
         * @return Oczekiwana lista wektorow stanowiaca trase lub blad.
         */
        std::expected<std::vector<Client::World::Vector3>, PathfindingError> FindPath(
            float startX, float startY, 
            float endX, float endY, 
            const Client::World::TerrainHeightSampler& sampler) const;

    private:
        static constexpr float GRID_SIZE = 100.0f; // Odstepy miedzy wezlami siatki
        static constexpr float MAX_SLOPE = 0.8f;   // Maksymalne nachylenie (wysokosc z / xy normalnej)

        GridPoint WorldToGrid(float x, float y) const;
        Client::World::Vector3 GridToWorld(const GridPoint& grid, const Client::World::TerrainHeightSampler& sampler) const;
        float CalculateHeuristic(const GridPoint& a, const GridPoint& b) const;
        bool IsPassable(const GridPoint& point, const Client::World::TerrainHeightSampler& sampler) const;
        
        std::vector<GridPoint> GetNeighbors(const GridPoint& point) const;
    };

} // namespace Client::Gameplay
