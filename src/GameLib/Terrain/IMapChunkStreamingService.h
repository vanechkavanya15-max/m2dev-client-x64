#pragma once

#include <cstdint>
#include <string_view>
#include <functional>
#include "TerrainCoordinates.h"

namespace GameLib::Terrain
{
    struct ChunkCoordinate
    {
        int32_t sectorX{0};
        int32_t sectorY{0};

        constexpr ChunkCoordinate() = default;
        constexpr ChunkCoordinate(int32_t sx, int32_t sy) noexcept : sectorX(sx), sectorY(sy) {}
        constexpr ChunkCoordinate(const SectorCoord& sc) noexcept : sectorX(sc.x), sectorY(sc.y) {}

        constexpr operator SectorCoord() const noexcept { return SectorCoord{sectorX, sectorY}; }

        bool operator==(const ChunkCoordinate&) const = default;
        auto operator<=>(const ChunkCoordinate&) const = default;
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

namespace std
{
    template<>
    struct hash<GameLib::Terrain::ChunkCoordinate>
    {
        std::size_t operator()(const GameLib::Terrain::ChunkCoordinate& coord) const noexcept
        {
            uint64_t packed = (static_cast<uint64_t>(static_cast<uint32_t>(coord.sectorX)) << 32) |
                               static_cast<uint32_t>(coord.sectorY);
            packed ^= packed >> 30;
            packed *= 0xbf58476d1ce4e5b9ULL;
            packed ^= packed >> 27;
            packed *= 0x94d049bb133111ebULL;
            packed ^= packed >> 31;
            return static_cast<size_t>(packed);
        }
    };
}
