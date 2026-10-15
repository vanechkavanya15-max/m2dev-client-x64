#include "../StdAfx.h"
#include "TerrainSubsystemFacade.h"
#include "../../EterBase/LogModern.h"

namespace GameLib::Terrain {

    std::unique_ptr<TerrainSubsystemFacade> CreateTerrainSubsystem() {
        return std::make_unique<TerrainSubsystemFacade>();
    }

    TerrainSubsystemFacade::TerrainSubsystemFacade()
        : m_streamingCoordinator(&m_heightStorage) {}

    void TerrainSubsystemFacade::LoadSectorData(
        SectorCoord coord, 
        std::vector<uint16_t> heights, 
        std::vector<uint8_t> waterHeights,
        float heightScale, 
        float baseHeight) 
    {
        // 1. Zbuduj i zarejestruj plaskie drzewo czworkowe sektora
        auto& tree = m_sectorTrees[coord];
        tree.BuildWithHeights(coord, heights, heightScale, baseHeight);

        // 2. Zaladuj dane do magazynu wysokosci
        ChunkHeightData data;
        data.heights = std::move(heights);
        data.waterHeights = std::move(waterHeights);
        data.heightScale = heightScale;
        data.baseHeight = baseHeight;

        m_heightStorage.InsertChunk(coord, std::move(data));
        m_streamingCoordinator.MarkChunkActive(coord);

        EterBase::ModernLogger::Info("TerrainSubsystemFacade: Loaded sector [{}, {}] successfully.", coord.x, coord.y);
    }

    void TerrainSubsystemFacade::UnloadSector(SectorCoord coord) {
        m_sectorTrees.erase(coord);
        m_heightStorage.RemoveChunk(coord);
    }

    float TerrainSubsystemFacade::SampleHeight(float worldX, float worldY) const {
        return m_heightStorage.SampleHeight(worldX, worldY);
    }

    std::optional<std::tuple<float, float, float>> TerrainSubsystemFacade::CalculateNormal(float worldX, float worldY) const {
        return m_heightStorage.CalculateNormal(WorldPosition{worldX, worldY, 0.0f});
    }

    bool TerrainSubsystemFacade::IsWalkable(float worldX, float worldY, float maxSlopeAngle) const {
        return m_heightStorage.IsWalkableSlope(worldX, worldY, maxSlopeAngle);
    }

    size_t TerrainSubsystemFacade::CullSectorPatches(SectorCoord coord, const float frustum[6][4], uint32_t* outVisiblePatchIds) {
        auto it = m_sectorTrees.find(coord);
        if (it != m_sectorTrees.end()) {
            return it->second.CullTerrainPatches(frustum, outVisiblePatchIds);
        }
        return 0;
    }

    void TerrainSubsystemFacade::Update(float playerX, float playerY) {
        m_streamingCoordinator.UpdateStreaming(playerX, playerY);
    }

    void TerrainSubsystemFacade::Clear() {
        m_sectorTrees.clear();
        m_streamingCoordinator.ClearAllChunks();
        m_heightStorage.Clear();
    }

} // namespace GameLib::Terrain
