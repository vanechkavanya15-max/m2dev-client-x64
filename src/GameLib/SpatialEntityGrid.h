#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cmath>

/**
 * @brief A 2D spatial hash grid for efficient proximity queries.
 *
 * This class partitions a 2D space into uniform grid cells to allow fast
 * localized lookups, primarily used for entity proximity detection (e.g., finding nearby actors).
 * 
 * @tparam Entity The type of the entity to store. Usually a uint32_t ID or a pointer.
 */
template <typename Entity>
class SpatialEntityGrid {
public:
    /**
     * @brief Constructor for the spatial hash grid.
     * @param cellSize The dimensions of a single square grid cell. Must be greater than 0.
     */
    explicit SpatialEntityGrid(float cellSize) : cellSize(cellSize) {
        if (this->cellSize <= 0.0f) {
            this->cellSize = 100.0f; // fallback to a default positive cell size
        }
    }

    /**
     * @brief Inserts an entity into the spatial grid at the given coordinates.
     * @param entity The entity to insert.
     * @param x The X coordinate of the entity.
     * @param y The Y coordinate of the entity.
     */
    void Insert(const Entity& entity, float x, float y) {
        auto cellId = GetCellId(x, y);
        cells[cellId].insert(entity);
    }

    /**
     * @brief Removes an entity from the spatial grid.
     * @param entity The entity to remove.
     * @param x The last known X coordinate of the entity.
     * @param y The last known Y coordinate of the entity.
     */
    void Remove(const Entity& entity, float x, float y) {
        auto cellId = GetCellId(x, y);
        auto it = cells.find(cellId);
        if (it != cells.end()) {
            it->second.erase(entity);
            if (it->second.empty()) {
                cells.erase(it);
            }
        }
    }

    /**
     * @brief Updates the position of an entity in the spatial grid.
     * @param entity The entity to update.
     * @param oldX The previous X coordinate.
     * @param oldY The previous Y coordinate.
     * @param newX The new X coordinate.
     * @param newY The new Y coordinate.
     */
    void Update(const Entity& entity, float oldX, float oldY, float newX, float newY) {
        auto oldCellId = GetCellId(oldX, oldY);
        auto newCellId = GetCellId(newX, newY);
        
        if (oldCellId != newCellId) {
            Remove(entity, oldX, oldY);
            Insert(entity, newX, newY);
        }
    }

    /**
     * @brief Clears all entities from the spatial grid.
     */
    void Clear() {
        cells.clear();
    }

    /**
     * @brief Finds all entities within a rough bounding box of the given radius.
     * 
     * This method returns all entities in cells that intersect with the bounding box
     * defined by (x - radius, y - radius) to (x + radius, y + radius).
     * 
     * @param x The center X coordinate.
     * @param y The center Y coordinate.
     * @param radius The radius defining the bounding box.
     * @return A vector containing all entities found in the overlapping cells.
     */
    [[nodiscard]] std::vector<Entity> FindNearby(float x, float y, float radius) const {
        std::vector<Entity> result;
        
        int32_t minX = GetGridCoord(x - radius);
        int32_t maxX = GetGridCoord(x + radius);
        int32_t minY = GetGridCoord(y - radius);
        int32_t maxY = GetGridCoord(y + radius);

        for (int32_t gridX = minX; gridX <= maxX; ++gridX) {
            for (int32_t gridY = minY; gridY <= maxY; ++gridY) {
                uint64_t cellId = GetHash(gridX, gridY);
                auto it = cells.find(cellId);
                if (it != cells.end()) {
                    for (const auto& entity : it->second) {
                        result.push_back(entity);
                    }
                }
            }
        }
        
        return result;
    }

    /**
     * @brief Finds entities within a rough bounding box, then filters them using a predicate.
     * 
     * Useful for performing exact circular distance checks (e.g., using Euclidean distance).
     * 
     * @tparam Predicate Callable that takes an Entity and returns true if it should be included.
     * @param x The center X coordinate.
     * @param y The center Y coordinate.
     * @param radius The radius defining the bounding box.
     * @param filter The predicate used to filter entities.
     * @return A vector of entities that pass the predicate.
     */
    template <typename Predicate>
    [[nodiscard]] std::vector<Entity> FindNearbyExact(float x, float y, float radius, Predicate filter) const {
        std::vector<Entity> result;
        
        int32_t minX = GetGridCoord(x - radius);
        int32_t maxX = GetGridCoord(x + radius);
        int32_t minY = GetGridCoord(y - radius);
        int32_t maxY = GetGridCoord(y + radius);

        for (int32_t gridX = minX; gridX <= maxX; ++gridX) {
            for (int32_t gridY = minY; gridY <= maxY; ++gridY) {
                uint64_t cellId = GetHash(gridX, gridY);
                auto it = cells.find(cellId);
                if (it != cells.end()) {
                    for (const auto& entity : it->second) {
                        if (filter(entity)) {
                            result.push_back(entity);
                        }
                    }
                }
            }
        }
        
        return result;
    }

private:
    float cellSize;
    std::unordered_map<uint64_t, std::unordered_set<Entity>> cells;

    /**
     * @brief Computes the 1D grid coordinate for a given continuous coordinate.
     * @param coord The continuous coordinate.
     * @return The integer grid coordinate.
     */
    [[nodiscard]] int32_t GetGridCoord(float coord) const {
        return static_cast<int32_t>(std::floor(coord / cellSize));
    }

    /**
     * @brief Computes a 64-bit hash from 2D grid coordinates.
     * @param gridX The grid X coordinate.
     * @param gridY The grid Y coordinate.
     * @return The 64-bit hash representing the cell ID.
     */
    [[nodiscard]] uint64_t GetHash(int32_t gridX, int32_t gridY) const {
        uint32_t ux = static_cast<uint32_t>(gridX);
        uint32_t uy = static_cast<uint32_t>(gridY);
        return (static_cast<uint64_t>(ux) << 32) | uy;
    }

    /**
     * @brief Gets the cell ID for a given continuous 2D position.
     * @param x The X coordinate.
     * @param y The Y coordinate.
     * @return The 64-bit cell ID.
     */
    [[nodiscard]] uint64_t GetCellId(float x, float y) const {
        return GetHash(GetGridCoord(x), GetGridCoord(y));
    }
};
