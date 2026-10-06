#include "../StdAfx.h"
#include <expected>
#include <cstdint>
#include <memory>
#include "IMapChunkStreamingService.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/StrongTypes.h"
#include "../../UserInterface/Core/EventBus.h"

namespace {
    struct MapChunkUploadedEvent : public UserInterface::Core::IEvent {
        int32_t sectorX;
        int32_t sectorY;

        MapChunkUploadedEvent(int32_t x, int32_t y) : sectorX(x), sectorY(y) {}
    };
}

namespace GameLib::Terrain {

    class MapChunkStreamingService final : public IMapChunkStreamingService {
    public:
        MapChunkStreamingService() = default;
        ~MapChunkStreamingService() override = default;

        void RequestChunk(ChunkCoordinate coord) override {
            EterBase::ModernLogger::Info("Uploading chunk to Direct3D Vertex Buffer for sector ({}, {})", coord.sectorX, coord.sectorY);
            UserInterface::Core::EventBus::GetInstance().Publish(MapChunkUploadedEvent(coord.sectorX, coord.sectorY));
        }

        bool IsChunkLoaded(ChunkCoordinate coord) const override {
            return false;
        }

        void UpdateStreaming(float playerX, float playerY) override {
        }

        void EvictDistantChunks(float playerX, float playerY, float maxRadius) override {
        }

        void ClearAllChunks() override {
        }
    };

}
