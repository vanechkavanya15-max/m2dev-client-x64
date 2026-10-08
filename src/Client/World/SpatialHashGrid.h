#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <cmath>
#include <shared_mutex>

#include "../../EterBase/StrongTypes.h"

namespace Client::World {

class SpatialHashGrid {
public:
    explicit SpatialHashGrid(float cellSize = 1024.0f);
    ~SpatialHashGrid() = default;

    void Insert(EterBase::EntityId id, float x, float y);
    void Update(EterBase::EntityId id, float x, float y);
    void Remove(EterBase::EntityId id);
    void Clear();
    [[nodiscard]] size_t Count() const;

    [[nodiscard]] std::vector<EterBase::EntityId> QueryRadius(float center_x, float center_y, float radius) const;

private:
    struct Position {
        float x;
        float y;
    };

    struct CellCoords {
        int x;
        int y;

        bool operator==(const CellCoords& other) const {
            return x == other.x && y == other.y;
        }
    };

    struct CellCoordsHash {
        std::size_t operator()(const CellCoords& coords) const {
            // Combine hashes using a simple but effective combination
            std::size_t h1 = std::hash<int>()(coords.x);
            std::size_t h2 = std::hash<int>()(coords.y);
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };

    mutable std::shared_mutex m_mutex;
    float m_cellSize;
    std::unordered_map<EterBase::EntityId, Position> m_entityPositions;
    std::unordered_map<CellCoords, std::vector<EterBase::EntityId>, CellCoordsHash> m_cells;

    CellCoords GetCellCoords(float x, float y) const;
};

} // namespace Client::World
