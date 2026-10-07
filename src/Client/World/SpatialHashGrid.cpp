#include "../../EterBase/StdAfx.h"
#include "SpatialHashGrid.h"

#include <algorithm>
#include <cmath>

namespace Client::World {

SpatialHashGrid::SpatialHashGrid(float cellSize) : m_cellSize(cellSize > 0.0f ? cellSize : 1024.0f) {
}

SpatialHashGrid::CellCoords SpatialHashGrid::GetCellCoords(float x, float y) const {
    return {
        static_cast<int>(std::floor(x / m_cellSize)),
        static_cast<int>(std::floor(y / m_cellSize))
    };
}

void SpatialHashGrid::Insert(EterBase::EntityId id, float x, float y) {
    if (m_entityPositions.contains(id)) {
        Update(id, x, y);
        return;
    }

    m_entityPositions[id] = {x, y};
    CellCoords coords = GetCellCoords(x, y);
    m_cells[coords].push_back(id);
}

void SpatialHashGrid::Update(EterBase::EntityId id, float x, float y) {
    auto it = m_entityPositions.find(id);
    if (it == m_entityPositions.end()) {
        Insert(id, x, y);
        return;
    }

    CellCoords oldCoords = GetCellCoords(it->second.x, it->second.y);
    CellCoords newCoords = GetCellCoords(x, y);

    it->second = {x, y};

    if (oldCoords == newCoords) {
        return; // Still in the same cell
    }

    // Remove from old cell
    auto& oldCell = m_cells[oldCoords];
    auto oldIt = std::find(oldCell.begin(), oldCell.end(), id);
    if (oldIt != oldCell.end()) {
        // Swap and pop for O(1) removal
        if (oldIt != oldCell.end() - 1) {
            std::iter_swap(oldIt, oldCell.end() - 1);
        }
        oldCell.pop_back();
    }

    // Add to new cell
    m_cells[newCoords].push_back(id);
}

void SpatialHashGrid::Remove(EterBase::EntityId id) {
    auto it = m_entityPositions.find(id);
    if (it == m_entityPositions.end()) {
        return;
    }

    CellCoords coords = GetCellCoords(it->second.x, it->second.y);
    auto& cell = m_cells[coords];

    auto cellIt = std::find(cell.begin(), cell.end(), id);
    if (cellIt != cell.end()) {
        if (cellIt != cell.end() - 1) {
            std::iter_swap(cellIt, cell.end() - 1);
        }
        cell.pop_back();
    }

    // Clean up empty cells if desired (optional optimization)
    if (cell.empty()) {
        m_cells.erase(coords);
    }

    m_entityPositions.erase(it);
}

std::vector<EterBase::EntityId> SpatialHashGrid::QueryRadius(float center_x, float center_y, float radius) const {
    std::vector<EterBase::EntityId> result;
    if (radius <= 0.0f) {
        return result;
    }

    float sqRadius = radius * radius;
    float minX = center_x - radius;
    float minY = center_y - radius;
    float maxX = center_x + radius;
    float maxY = center_y + radius;

    CellCoords minCoords = GetCellCoords(minX, minY);
    CellCoords maxCoords = GetCellCoords(maxX, maxY);

    for (int y = minCoords.y; y <= maxCoords.y; ++y) {
        for (int x = minCoords.x; x <= maxCoords.x; ++x) {
            auto it = m_cells.find({x, y});
            if (it != m_cells.end()) {
                for (EterBase::EntityId id : it->second) {
                    auto posIt = m_entityPositions.find(id);
                    if (posIt != m_entityPositions.end()) {
                        float dx = posIt->second.x - center_x;
                        float dy = posIt->second.y - center_y;
                        if (dx * dx + dy * dy <= sqRadius) {
                            result.push_back(id);
                        }
                    }
                }
            }
        }
    }

    return result;
}

} // namespace Client::World
