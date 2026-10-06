#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <span>

/**
 * @brief A fast spatial query system for circle-based area lookups.
 * 
 * This class uses a spatial grid partitioning system to optimize finding
 * objects (identifiers) within a specific 2D radius, reducing the time complexity
 * from O(N) to roughly O(K) where K is the number of items in nearby cells.
 */
class CircleAreaQuery {
public:
    /**
     * @brief Constructs the query system with a specified grid cell size.
     * @param cellSize The size of each square grid cell (must be > 0.0f).
     */
    explicit CircleAreaQuery(float cellSize = 1000.0f) 
        : cellSize(cellSize > 0.0f ? cellSize : 1000.0f) {}

    /**
     * @brief Adds or updates an entity's position in the spatial registry.
     * @param id The unique identifier of the entity.
     * @param x The X coordinate.
     * @param y The Y coordinate.
     */
    void UpdateEntity(uint32_t id, float x, float y) {
        RemoveEntity(id); // Ensure it is removed from any previous cell
        
        int32_t cellX = static_cast<int32_t>(std::floor(x / cellSize));
        int32_t cellY = static_cast<int32_t>(std::floor(y / cellSize));
        uint64_t cellKey = GetCellKey(cellX, cellY);

        cells[cellKey].push_back(id);
        entityPositions[id] = {x, y, cellKey};
    }

    /**
     * @brief Removes an entity from the spatial registry.
     * @param id The unique identifier of the entity.
     */
    void RemoveEntity(uint32_t id) {
        auto it = entityPositions.find(id);
        if (it != entityPositions.end()) {
            uint64_t cellKey = it->second.cellKey;
            auto& cellEntities = cells[cellKey];
            
            std::erase(cellEntities, id);
            
            if (cellEntities.empty()) {
                cells.erase(cellKey);
            }
            entityPositions.erase(it);
        }
    }

    /**
     * @brief Clears all entities from the spatial registry.
     */
    void Clear() {
        cells.clear();
        entityPositions.clear();
    }

    /**
     * @brief Finds all entity identifiers within a specified radius from a center point.
     * @param centerX The X coordinate of the circle's center.
     * @param centerY The Y coordinate of the circle's center.
     * @param radius The radius of the query circle.
     * @return A vector of entity identifiers that fall within the specified radius.
     */
    [[nodiscard]] std::vector<uint32_t> FindInRadius(float centerX, float centerY, float radius) const {
        std::vector<uint32_t> result;
        if (radius < 0.0f) {
            return result;
        }

        const float radiusSquared = radius * radius;
        
        const int32_t minCellX = static_cast<int32_t>(std::floor((centerX - radius) / cellSize));
        const int32_t maxCellX = static_cast<int32_t>(std::floor((centerX + radius) / cellSize));
        const int32_t minCellY = static_cast<int32_t>(std::floor((centerY - radius) / cellSize));
        const int32_t maxCellY = static_cast<int32_t>(std::floor((centerY + radius) / cellSize));

        for (int32_t cx = minCellX; cx <= maxCellX; ++cx) {
            for (int32_t cy = minCellY; cy <= maxCellY; ++cy) {
                uint64_t cellKey = GetCellKey(cx, cy);
                auto cellIt = cells.find(cellKey);
                if (cellIt != cells.end()) {
                    for (uint32_t id : cellIt->second) {
                        auto posIt = entityPositions.find(id);
                        if (posIt != entityPositions.end()) {
                            const float dx = posIt->second.x - centerX;
                            const float dy = posIt->second.y - centerY;
                            const float distanceSquared = (dx * dx) + (dy * dy);

                            if (distanceSquared <= radiusSquared) {
                                result.push_back(id);
                            }
                        }
                    }
                }
            }
        }

        return result;
    }

private:
    struct EntityData {
        float x;
        float y;
        uint64_t cellKey;
    };

    float cellSize;
    std::unordered_map<uint64_t, std::vector<uint32_t>> cells;
    std::unordered_map<uint32_t, EntityData> entityPositions;

    /**
     * @brief Generates a unique 64-bit key for a 2D grid cell.
     * @param cellX The X index of the cell.
     * @param cellY The Y index of the cell.
     * @return A unique 64-bit integer representing the cell.
     */
    [[nodiscard]] constexpr uint64_t GetCellKey(int32_t cellX, int32_t cellY) const noexcept {
        return (static_cast<uint64_t>(static_cast<uint32_t>(cellX)) << 32) | static_cast<uint32_t>(cellY);
    }
};
