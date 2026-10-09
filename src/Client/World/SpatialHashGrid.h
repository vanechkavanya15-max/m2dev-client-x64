#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <cmath>
#include <shared_mutex>
#include <optional>

#include "../../EterBase/StrongTypes.h"

namespace Client::World {

class SpatialHashGrid {
public:
    explicit SpatialHashGrid(float cellSize = 1024.0f);
    ~SpatialHashGrid() = default;

    void Insert(EterBase::EntityId id, float x, float y);
    void Update(EterBase::EntityId id, float x, float y);
    bool UpdateIfMoved(EterBase::EntityId id, float x, float y);
    void Remove(EterBase::EntityId id);
    void Clear();
    [[nodiscard]] size_t Count() const;

    [[nodiscard]] std::vector<EterBase::EntityId> QueryRadius(float center_x, float center_y, float radius) const;

    template <typename VisitorFn>
    void QueryRadiusVisitor(float center_x, float center_y, float radius, VisitorFn&& visitor) const {
        if (radius <= 0.0f) {
            return;
        }
        std::shared_lock lock(m_mutex);
        float sqRadius = radius * radius;
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
                                visitor(id, posIt->second.x, posIt->second.y);
                            }
                        }
                    }
                }
            }
        }
    }

    [[nodiscard]] std::optional<EterBase::EntityId> QueryNearest(float center_x, float center_y, float maxRadius, std::optional<EterBase::EntityId> ignoreId = std::nullopt) const;

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
