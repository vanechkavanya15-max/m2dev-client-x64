#pragma once

#include "../../UserInterface/Core/EventBus.h"
#include "../../EterBase/StrongTypes.h"
#include "TerrainCoordinates.h"
#include "IMapChunkStreamingService.h"
#include <vector>
#include <cstdint>

namespace GameLib::Terrain {

    struct ChunkLoadedEvent : public UserInterface::Core::IEvent {
        ChunkCoordinate coord;
        std::vector<uint16_t> heightMap;
        std::vector<uint8_t> splatMap;

        ChunkLoadedEvent(ChunkCoordinate c) : coord(c) {}
        ChunkLoadedEvent(ChunkCoordinate c, std::vector<uint16_t> hm, std::vector<uint8_t> sm)
            : coord(c), heightMap(std::move(hm)), splatMap(std::move(sm)) {}
    };

    struct ChunkEvictedEvent : public UserInterface::Core::IEvent {
        ChunkCoordinate coord;
        explicit ChunkEvictedEvent(ChunkCoordinate c) : coord(c) {}
    };

    struct QuadtreeBuiltEvent : public UserInterface::Core::IEvent {
        int32_t sectorX{0};
        int32_t sectorY{0};
        size_t totalNodes{0};

        QuadtreeBuiltEvent(int32_t x, int32_t y, size_t count)
            : sectorX(x), sectorY(y), totalNodes(count) {}
    };

    struct TerrainCullCompletedEvent : public UserInterface::Core::IEvent {
        size_t visibleCount{0};
        explicit TerrainCullCompletedEvent(size_t count = 0) : visibleCount(count) {}
    };

    struct TerrainHeightCacheClearedEvent : public UserInterface::Core::IEvent {
        TerrainHeightCacheClearedEvent() = default;
    };

    struct TerrainNormalCalculatedEvent : public UserInterface::Core::IEvent {
        EterBase::EntityId entityId;
        float normalX{0.0f};
        float normalY{0.0f};
        float normalZ{1.0f};

        TerrainNormalCalculatedEvent(EterBase::EntityId id, float x, float y, float z)
            : entityId(id), normalX(x), normalY(y), normalZ(z) {}
    };

    struct TerrainBoundaryStitchedEvent : public UserInterface::Core::IEvent {
        ChunkCoordinate chunkA;
        ChunkCoordinate chunkB;
        uint32_t stitchedVerticesCount{0};

        TerrainBoundaryStitchedEvent(ChunkCoordinate a, ChunkCoordinate b, uint32_t count)
            : chunkA(a), chunkB(b), stitchedVerticesCount(count) {}
    };

} // namespace GameLib::Terrain
