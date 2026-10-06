#pragma once

#include <cstdint>
#include <string_view>

namespace GameLib::Terrain
{
    struct ChunkCoordinate
    {
        int32_t sectorX{0};
        int32_t sectorY{0};
    };

    class IMapChunkStreamingService
    {
    public:
        virtual ~IMapChunkStreamingService() = default;

        virtual void RequestChunk(ChunkCoordinate coord) = 0;
        virtual bool IsChunkLoaded(ChunkCoordinate coord) const = 0;
        virtual void UpdateStreaming(float playerX, float playerY) = 0;
        virtual void EvictDistantChunks(float playerX, float playerY, float maxRadius) = 0;
        virtual void ClearAllChunks() = 0;
    };
}
