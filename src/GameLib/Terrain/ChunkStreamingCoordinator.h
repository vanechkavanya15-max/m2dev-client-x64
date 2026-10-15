#pragma once

#include "IMapChunkStreamingService.h"
#include "TerrainCoordinates.h"
#include "TerrainHeightStorage.h"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>

namespace GameLib::Terrain {

    enum class ChunkState : uint8_t {
        Unloaded,
        Queued,
        Loading,
        Active,
        Evicting
    };

    struct ChunkSlot {
        SectorCoord coord;
        ChunkState state{ChunkState::Unloaded};
        float loadPriority{0.0f};
    };

    class ChunkStreamingCoordinator final : public IMapChunkStreamingService {
    public:
        explicit ChunkStreamingCoordinator(TerrainHeightStorage* heightStorage = nullptr);
        ~ChunkStreamingCoordinator() override = default;

        void SetHeightStorage(TerrainHeightStorage* storage) noexcept { m_heightStorage = storage; }

        // Implementacja IMapChunkStreamingService
        void RequestChunk(ChunkCoordinate coord) override;
        bool IsChunkLoaded(ChunkCoordinate coord) const override;
        void UpdateStreaming(float playerX, float playerY) override;
        void EvictDistantChunks(float playerX, float playerY, float maxRadius) override;
        void ClearAllChunks() override;

        // Metody zarzadzania stanem kafelka
        void MarkChunkActive(SectorCoord coord);
        ChunkState GetChunkState(SectorCoord coord) const noexcept;
        std::vector<SectorCoord> GetPendingLoadQueue();

    private:
        void EnsureSlot(SectorCoord coord, WorldPosition playerPos, WorldPosition playerVel);

        mutable std::mutex m_mutex;
        TerrainHeightStorage* m_heightStorage{nullptr};
        std::unordered_map<SectorCoord, ChunkSlot> m_chunks;

        bool m_hasLastPos{false};
        WorldPosition m_lastPlayerPos{};
    };

    std::unique_ptr<IMapChunkStreamingService> CreateMapChunkStreamingService(TerrainHeightStorage* heightStorage = nullptr);

} // namespace GameLib::Terrain
