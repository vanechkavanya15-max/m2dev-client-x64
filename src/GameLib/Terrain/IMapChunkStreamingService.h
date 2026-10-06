#pragma once

#include <cstdint>
#include <string_view>

namespace GameLib::Terrain
{
    struct ChunkCoordinate
    {
        int32_t sectorX{0};
        int32_t sectorY{0};

        bool operator==(const ChunkCoordinate&) const = default;
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

#include <functional>

namespace std
{
    template<>
    struct hash<GameLib::Terrain::ChunkCoordinate>
    {
        std::size_t operator()(const GameLib::Terrain::ChunkCoordinate& coord) const noexcept
        {
            std::size_t h1 = std::hash<int32_t>{}(coord.sectorX);
            std::size_t h2 = std::hash<int32_t>{}(coord.sectorY);
            return h1 ^ (h2 << 1);
        }
    };
}
