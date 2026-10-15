#include "../StdAfx.h"
#include "TerrainHeightStorage.h"
#include "TerrainEvents.h"
#include "../../EterBase/LogModern.h"
#include <algorithm>
#include <cmath>

namespace GameLib::Terrain {

    std::unique_ptr<ITerrainHeightCache> CreateTerrainHeightCache() {
        return std::make_unique<TerrainHeightStorage>();
    }

    std::unique_ptr<ITerrainHeightCache> CreateTerrainHeightGridCache() {
        return std::make_unique<TerrainHeightStorage>();
    }

    std::expected<std::unique_ptr<ITerrainHeightCache>, EterBase::EntityError> CreateTerrainHeightMemoryPool() {
        return std::make_unique<TerrainHeightStorage>();
    }

    float ChunkHeightData::SampleHeightTriangular(float localX, float localY, float* outXslo, float* outYslo) const noexcept {
        if (localX < 0.0f || localY < 0.0f || 
            localX > TerrainMetrics::SectorSize || localY > TerrainMetrics::SectorSize) {
            if (outXslo) *outXslo = 0.0f;
            if (outYslo) *outYslo = 0.0f;
            return baseHeight;
        }

        uint32_t cellX = static_cast<uint32_t>(localX / TerrainMetrics::CellSize);
        uint32_t cellY = static_cast<uint32_t>(localY / TerrainMetrics::CellSize);

        cellX = std::min(cellX, TerrainMetrics::CellsPerSector - 1);
        cellY = std::min(cellY, TerrainMetrics::CellsPerSector - 1);

        float xdist = localX - (static_cast<float>(cellX) * TerrainMetrics::CellSize);
        float ydist = localY - (static_cast<float>(cellY) * TerrainMetrics::CellSize);
        constexpr float ooscale = 1.0f / TerrainMetrics::CellSize;

        // Przeliczenie na surowy bufor 131x131 (+1 margines brzegowy)
        uint32_t rawX = cellX + TerrainMetrics::RawBorderOffset;
        uint32_t rawY = cellY + TerrainMetrics::RawBorderOffset;

        float h1 = GetRawHeight(rawX,     rawY);     // Top-Left
        float h2 = GetRawHeight(rawX + 1, rawY + 1); // Bottom-Right

        float xslope = 0.0f;
        float yslope = 0.0f;
        float height = h1;

        // Podzial komorki na 2 trojkaty dokladnie jak w AreaTerrain.cpp (linie 408-430)
        if (xdist <= ydist) {
            // Lewy trojkat: Top-Left, Bottom-Right, Bottom-Left
            float h3 = GetRawHeight(rawX, rawY + 1); // Bottom-Left
            xslope = (h2 - h3) * ooscale;
            yslope = (h3 - h1) * ooscale;
            height = h1 + (xdist * xslope + ydist * yslope);
        } else {
            // Prawy trojkat: Top-Left, Top-Right, Bottom-Right
            float h3 = GetRawHeight(rawX + 1, rawY); // Top-Right
            xslope = (h3 - h1) * ooscale;
            yslope = (h2 - h3) * ooscale;
            height = h1 + (xdist * xslope + ydist * yslope);
        }

        if (outXslo) *outXslo = xslope;
        if (outYslo) *outYslo = yslope;
        return height;
    }

    void TerrainHeightStorage::InsertChunk(SectorCoord coord, ChunkHeightData data) {
        std::unique_lock lock(m_mutex);
        m_chunks[coord] = std::make_shared<ChunkHeightData>(std::move(data));
    }

    void TerrainHeightStorage::RemoveChunk(SectorCoord coord) {
        std::unique_lock lock(m_mutex);
        m_chunks.erase(coord);
    }

    bool TerrainHeightStorage::HasChunk(SectorCoord coord) const noexcept {
        std::shared_lock lock(m_mutex);
        return m_chunks.contains(coord);
    }

    float TerrainHeightStorage::SampleHeight(float x, float y) const {
        auto h = SampleHeightExact(WorldPosition{x, y, 0.0f});
        return h.value_or(0.0f);
    }

    float TerrainHeightStorage::SampleWaterHeight(float x, float y) const {
        WorldPosition pos{x, y, 0.0f};
        if (!pos.IsFinite()) return -10000.0f;

        SectorCoord sector = TerrainMetrics::WorldToSector(pos);
        WorldPosition origin = TerrainMetrics::SectorToWorld(sector);
        float localX = pos.x - origin.x;
        float localY = pos.y - origin.y;

        uint32_t cellX = static_cast<uint32_t>(localX / TerrainMetrics::CellSize);
        uint32_t cellY = static_cast<uint32_t>(localY / TerrainMetrics::CellSize);
        cellX = std::min(cellX, TerrainMetrics::CellsPerSector - 1);
        cellY = std::min(cellY, TerrainMetrics::CellsPerSector - 1);

        uint32_t rawX = cellX + TerrainMetrics::RawBorderOffset;
        uint32_t rawY = cellY + TerrainMetrics::RawBorderOffset;

        std::shared_lock lock(m_mutex);
        auto it = m_chunks.find(sector);
        if (it == m_chunks.end()) return -10000.0f;
        return it->second->GetWaterHeight(rawX, rawY);
    }

    bool TerrainHeightStorage::IsWalkableSlope(float x, float y, float maxSlopeAngle) const {
        WorldPosition pos{x, y, 0.0f};
        if (!pos.IsFinite()) return false;

        SectorCoord sector = TerrainMetrics::WorldToSector(pos);
        WorldPosition origin = TerrainMetrics::SectorToWorld(sector);
        float localX = pos.x - origin.x;
        float localY = pos.y - origin.y;

        float xslope = 0.0f, yslope = 0.0f;
        {
            std::shared_lock lock(m_mutex);
            auto it = m_chunks.find(sector);
            if (it == m_chunks.end()) return true;
            it->second->SampleHeightTriangular(localX, localY, &xslope, &yslope);
        }

        float gradient = std::sqrt(xslope * xslope + yslope * yslope);
        float slopeAngle = std::atan(gradient) * (180.0f / 3.14159265358979323846f);
        return slopeAngle <= maxSlopeAngle;
    }

    void TerrainHeightStorage::BatchSampleHeight(const float* x, const float* y, float* outZ, size_t count) const {
        if (!x || !y || !outZ || count == 0) return;

        std::shared_lock lock(m_mutex);
        SectorCoord lastSector{std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min()};
        const ChunkHeightData* cachedData = nullptr;
        WorldPosition cachedOrigin{};

        for (size_t i = 0; i < count; ++i) {
            WorldPosition pos{x[i], y[i], 0.0f};
            if (!pos.IsFinite()) {
                outZ[i] = 0.0f;
                continue;
            }

            SectorCoord sector = TerrainMetrics::WorldToSector(pos);
            if (sector != lastSector) {
                lastSector = sector;
                auto it = m_chunks.find(sector);
                if (it != m_chunks.end()) {
                    cachedData = it->second.get();
                    cachedOrigin = TerrainMetrics::SectorToWorld(sector);
                } else {
                    cachedData = nullptr;
                }
            }

            if (cachedData) {
                outZ[i] = cachedData->SampleHeightTriangular(pos.x - cachedOrigin.x, pos.y - cachedOrigin.y);
            } else {
                outZ[i] = 0.0f;
            }
        }
    }

    void TerrainHeightStorage::Clear() {
        {
            std::unique_lock lock(m_mutex);
            m_chunks.clear();
        }
        UserInterface::Core::EventBus::GetInstance().Publish(TerrainHeightCacheClearedEvent{});
    }

    std::optional<float> TerrainHeightStorage::SampleHeightExact(WorldPosition pos) const noexcept {
        if (!pos.IsFinite()) return std::nullopt;

        SectorCoord sector = TerrainMetrics::WorldToSector(pos);
        WorldPosition origin = TerrainMetrics::SectorToWorld(sector);
        float localX = pos.x - origin.x;
        float localY = pos.y - origin.y;

        std::shared_lock lock(m_mutex);
        auto it = m_chunks.find(sector);
        if (it == m_chunks.end()) return std::nullopt;

        // Odczyt bezposrednio przez surowy wskaznik pod lockiem bez kopiowania shared_ptr
        return it->second->SampleHeightTriangular(localX, localY);
    }

    std::optional<std::tuple<float, float, float>> TerrainHeightStorage::CalculateNormal(WorldPosition pos) const noexcept {
        if (!pos.IsFinite()) return std::nullopt;

        SectorCoord sector = TerrainMetrics::WorldToSector(pos);
        WorldPosition origin = TerrainMetrics::SectorToWorld(sector);
        float localX = pos.x - origin.x;
        float localY = pos.y - origin.y;

        float xslope = 0.0f, yslope = 0.0f;
        {
            std::shared_lock lock(m_mutex);
            auto it = m_chunks.find(sector);
            if (it == m_chunks.end()) return std::nullopt;
            it->second->SampleHeightTriangular(localX, localY, &xslope, &yslope);
        }

        // Analityczna normalna plaszczyzny trojkata (-xslope, -yslope, 1.0f)
        float len = std::sqrt(xslope * xslope + yslope * yslope + 1.0f);
        return std::tuple{-xslope / len, -yslope / len, 1.0f / len};
    }

} // namespace GameLib::Terrain
