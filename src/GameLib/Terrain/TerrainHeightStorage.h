#pragma once

#include "ITerrainHeightCache.h"
#include "TerrainCoordinates.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <optional>
#include <tuple>
#include <span>

namespace GameLib::Terrain {

    struct ChunkHeightData {
        std::vector<uint16_t> heights;      // 131 x 131 w formacie Metin2
        std::vector<uint8_t> waterHeights;  // 131 x 131 lub 128 x 128
        float heightScale{1.0f};
        float baseHeight{0.0f};

        [[nodiscard]] float GetRawHeight(uint32_t rawX, uint32_t rawY) const noexcept {
            if (rawX >= TerrainMetrics::HeightmapSize || rawY >= TerrainMetrics::HeightmapSize || heights.empty()) {
                return baseHeight;
            }
            uint32_t idx = rawY * TerrainMetrics::HeightmapSize + rawX;
            if (idx >= heights.size()) return baseHeight;
            return baseHeight + (static_cast<float>(heights[idx]) * heightScale);
        }

        [[nodiscard]] float GetWaterHeight(uint32_t rawX, uint32_t rawY) const noexcept {
            if (waterHeights.empty()) return -10000.0f;
            uint32_t size = static_cast<uint32_t>(std::sqrt(waterHeights.size()));
            if (size == 0) return -10000.0f;
            uint32_t x = std::min(rawX, size - 1);
            uint32_t y = std::min(rawY, size - 1);
            return static_cast<float>(waterHeights[y * size + x]);
        }

        // Dokladne probkowanie 2 trojkatow w komorce siatki (1:1 z Direct3D i AreaTerrain.cpp)
        float SampleHeightTriangular(float localX, float localY, float* outXslo = nullptr, float* outYslo = nullptr) const noexcept;
    };

    class TerrainHeightStorage final : public ITerrainHeightCache {
    public:
        TerrainHeightStorage() = default;
        ~TerrainHeightStorage() override = default;

        void InsertChunk(SectorCoord coord, ChunkHeightData data);
        void RemoveChunk(SectorCoord coord);
        bool HasChunk(SectorCoord coord) const noexcept;

        // Implementacja ITerrainHeightCache
        float SampleHeight(float x, float y) const override;
        float SampleWaterHeight(float x, float y) const override;
        bool IsWalkableSlope(float x, float y, float maxSlopeAngle) const override;
        void BatchSampleHeight(const float* x, const float* y, float* outZ, size_t count) const override;
        void Clear() override;

        // Rozszerzone API specyficzne dla domeny terenu
        [[nodiscard]] std::optional<float> SampleHeightExact(WorldPosition pos) const noexcept;
        [[nodiscard]] std::optional<std::tuple<float, float, float>> CalculateNormal(WorldPosition pos) const noexcept;

    private:
        mutable std::shared_mutex m_mutex;
        std::unordered_map<SectorCoord, std::shared_ptr<ChunkHeightData>> m_chunks;
    };

} // namespace GameLib::Terrain
