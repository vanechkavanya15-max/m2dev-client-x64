#include "SpatialHashGrid.h"

#include <algorithm>
#include <cmath>
#include <mutex>

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
    std::unique_lock lock(m_mutex);
    if (m_entityPositions.contains(id)) {
        lock.unlock();
        Update(id, x, y);
        return;
    }

    m_entityPositions[id] = {x, y};
    CellCoords coords = GetCellCoords(x, y);
    m_cells[coords].push_back(id);
}

void SpatialHashGrid::Update(EterBase::EntityId id, float x, float y) {
    std::unique_lock lock(m_mutex);
    auto it = m_entityPositions.find(id);
    if (it == m_entityPositions.end()) {
        m_entityPositions[id] = {x, y};
        CellCoords coords = GetCellCoords(x, y);
        m_cells[coords].push_back(id);
        return;
    }

    CellCoords oldCoords = GetCellCoords(it->second.x, it->second.y);
    CellCoords newCoords = GetCellCoords(x, y);

    it->second = {x, y};

    if (oldCoords == newCoords) {
        return; // Still in the same cell
    }

    // Remove from old cell
    auto oldCellIt = m_cells.find(oldCoords);
    if (oldCellIt != m_cells.end()) {
        auto& oldCell = oldCellIt->second;
        auto oldIt = std::find(oldCell.begin(), oldCell.end(), id);
        if (oldIt != oldCell.end()) {
            // Swap and pop for O(1) removal
            if (oldIt != oldCell.end() - 1) {
                std::iter_swap(oldIt, oldCell.end() - 1);
            }
            oldCell.pop_back();
        }
        if (oldCell.empty()) {
            m_cells.erase(oldCellIt);
        }
    }

    // Add to new cell
    m_cells[newCoords].push_back(id);
}

void SpatialHashGrid::Remove(EterBase::EntityId id) {
    std::unique_lock lock(m_mutex);
    auto it = m_entityPositions.find(id);
    if (it == m_entityPositions.end()) {
        return;
    }

    CellCoords coords = GetCellCoords(it->second.x, it->second.y);
    auto cellItMap = m_cells.find(coords);
    if (cellItMap != m_cells.end()) {
        auto& cell = cellItMap->second;
        auto cellIt = std::find(cell.begin(), cell.end(), id);
        if (cellIt != cell.end()) {
            if (cellIt != cell.end() - 1) {
                std::iter_swap(cellIt, cell.end() - 1);
            }
            cell.pop_back();
        }

        if (cell.empty()) {
            m_cells.erase(cellItMap);
        }
    }

    m_entityPositions.erase(it);
}

void SpatialHashGrid::Clear() {
    std::unique_lock lock(m_mutex);
    m_entityPositions.clear();
    m_cells.clear();
}

size_t SpatialHashGrid::Count() const {
    std::shared_lock lock(m_mutex);
    return m_entityPositions.size();
}

std::vector<EterBase::EntityId> SpatialHashGrid::QueryRadius(float center_x, float center_y, float radius) const {
    std::shared_lock lock(m_mutex);
    std::vector<EterBase::EntityId> result;
    if (radius <= 0.0f) {
        return result;
    }
    result.reserve(32);

    float sqRadius = radius * radius;
    // Apply small epsilon margin to avoid missing boundary cells due to float precision
    constexpr float eps = 0.001f;
    float minX = center_x - radius - eps;
    float minY = center_y - radius - eps;
    float maxX = center_x + radius + eps;
    float maxY = center_y + radius + eps;

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

std::optional<EterBase::EntityId> SpatialHashGrid::QueryNearest(
    float center_x,
    float center_y,
    float maxRadius,
    std::optional<EterBase::EntityId> ignoreId
) const {
    if (maxRadius <= 0.0f) {
        return std::nullopt;
    }

    std::shared_lock lock(m_mutex);

    constexpr float eps = 0.001f;
    float minX = center_x - maxRadius - eps;
    float minY = center_y - maxRadius - eps;
    float maxX = center_x + maxRadius + eps;
    float maxY = center_y + maxRadius + eps;

    CellCoords minCoords = GetCellCoords(minX, minY);
    CellCoords maxCoords = GetCellCoords(maxX, maxY);

    float bestDistSq = maxRadius * maxRadius;
    std::optional<EterBase::EntityId> nearestId = std::nullopt;

    for (int y = minCoords.y; y <= maxCoords.y; ++y) {
        for (int x = minCoords.x; x <= maxCoords.x; ++x) {
            auto it = m_cells.find({x, y});
            if (it != m_cells.end()) {
                for (EterBase::EntityId id : it->second) {
                    if (ignoreId.has_value() && id == *ignoreId) {
                        continue;
                    }
                    auto posIt = m_entityPositions.find(id);
                    if (posIt != m_entityPositions.end()) {
                        float dx = posIt->second.x - center_x;
                        float dy = posIt->second.y - center_y;
                        float distSq = dx * dx + dy * dy;
                        if (distSq <= bestDistSq) {
                            bestDistSq = distSq;
                            nearestId = id;
                        }
                    }
                }
            }
        }
    }

    return nearestId;
}

} // namespace Client::World
