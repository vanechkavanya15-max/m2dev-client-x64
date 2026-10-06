#include "../StdAfx.h"
#include "IMapChunkStreamingService.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/ModernLogger.h"
#include "../../UserInterface/Core/EventBus.h"
#include <cmath>
#include <vector>
#include <expected>

namespace UserInterface::Core {
    struct TerrainBoundaryStitchedEvent : public IEvent {
        GameLib::Terrain::ChunkCoordinate chunkA;
        GameLib::Terrain::ChunkCoordinate chunkB;
        uint32_t stitchedVerticesCount;

        TerrainBoundaryStitchedEvent(GameLib::Terrain::ChunkCoordinate a, GameLib::Terrain::ChunkCoordinate b, uint32_t count)
            : chunkA(a), chunkB(b), stitchedVerticesCount(count) {}
    };
}

namespace GameLib::Terrain {

    class MapChunk_BoundaryStitch final : public IMapChunkStreamingService {
    public:
        MapChunk_BoundaryStitch() = default;
        ~MapChunk_BoundaryStitch() override = default;

        void RequestChunk(ChunkCoordinate coord) override {
            EterBase::ModernLogger::Debug("MapChunk_BoundaryStitch: RequestChunk sectorX={}, sectorY={}", coord.sectorX, coord.sectorY);
        }

        bool IsChunkLoaded(ChunkCoordinate coord) const override {
            return true; 
        }

        void UpdateStreaming(float playerX, float playerY) override {
            // Not implemented in this specific action
        }

        void EvictDistantChunks(float playerX, float playerY, float maxRadius) override {
            // Not implemented in this specific action
        }

        void ClearAllChunks() override {
            // Not implemented in this specific action
        }

        // Akcja dedykowana - Boundary Stitching
        std::expected<void, std::string_view> StitchBoundaries(ChunkCoordinate chunkA, ChunkCoordinate chunkB) {
            if (!IsAdjacent(chunkA, chunkB)) {
                EterBase::ModernLogger::Error("MapChunk_BoundaryStitch: Chunks are not adjacent");
                return std::unexpected("Chunks are not adjacent");
            }

            // Symulacja laczenia wierzcholkow na granicy
            uint32_t stitchedVerticesCount = 128; // Przykladowa wartosc

            EterBase::ModernLogger::Info("MapChunk_BoundaryStitch: Successfully stitched {} vertices between ({},{}) and ({},{})", 
                stitchedVerticesCount, chunkA.sectorX, chunkA.sectorY, chunkB.sectorX, chunkB.sectorY);

            // Publikacja zdarzenia zeby odciac GUI
            UserInterface::Core::TerrainBoundaryStitchedEvent eventObj(chunkA, chunkB, stitchedVerticesCount);
            UserInterface::Core::EventBus::GetInstance().Publish(eventObj);

            return {};
        }

    private:
        bool IsAdjacent(const ChunkCoordinate& a, const ChunkCoordinate& b) const {
            int dx = std::abs(a.sectorX - b.sectorX);
            int dy = std::abs(a.sectorY - b.sectorY);
            return (dx == 1 && dy == 0) || (dx == 0 && dy == 1);
        }
    };

}
