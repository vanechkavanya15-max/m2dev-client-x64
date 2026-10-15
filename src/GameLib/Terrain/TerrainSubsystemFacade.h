#pragma once

#include "TerrainCoordinates.h"
#include "TerrainHeightStorage.h"
#include "LinearSectorQuadtree.h"
#include "ChunkStreamingCoordinator.h"
#include <memory>
#include <unordered_map>

namespace GameLib::Terrain {

    class TerrainSubsystemFacade {
    public:
        TerrainSubsystemFacade();
        ~TerrainSubsystemFacade() = default;

        // Dostep do komponentow
        [[nodiscard]] TerrainHeightStorage& GetHeightStorage() noexcept { return m_heightStorage; }
        [[nodiscard]] const TerrainHeightStorage& GetHeightStorage() const noexcept { return m_heightStorage; }

        [[nodiscard]] ChunkStreamingCoordinator& GetStreamingCoordinator() noexcept { return m_streamingCoordinator; }
        [[nodiscard]] const ChunkStreamingCoordinator& GetStreamingCoordinator() const noexcept { return m_streamingCoordinator; }

        // Zarzadzanie kafelkami i drzewami czworkowymi
        void LoadSectorData(
            SectorCoord coord, 
            std::vector<uint16_t> heights, 
            std::vector<uint8_t> waterHeights = {},
            float heightScale = 1.0f, 
            float baseHeight = 0.0f);

        void UnloadSector(SectorCoord coord);

        // Operacje renderowania i fizyki
        [[nodiscard]] float SampleHeight(float worldX, float worldY) const;
        [[nodiscard]] std::optional<std::tuple<float, float, float>> CalculateNormal(float worldX, float worldY) const;
        [[nodiscard]] bool IsWalkable(float worldX, float worldY, float maxSlopeAngle) const;

        size_t CullSectorPatches(SectorCoord coord, const float frustum[6][4], uint32_t* outVisiblePatchIds);

        void Update(float playerX, float playerY);
        void Clear();

    private:
        TerrainHeightStorage m_heightStorage;
        ChunkStreamingCoordinator m_streamingCoordinator;
        std::unordered_map<SectorCoord, LinearSectorQuadtree> m_sectorTrees;
    };

    std::unique_ptr<TerrainSubsystemFacade> CreateTerrainSubsystem();

} // namespace GameLib::Terrain
