#pragma once

#include "../../UserInterface/Core/EventBus.h"
#include "../../EterBase/StrongTypes.h"
#include "IMapChunkStreamingService.h"
#include <vector>
#include <cstdint>

namespace GameLib::Terrain::Events
{
    /**
     * @brief Event published when a map chunk is successfully loaded and decompressed.
     */
    struct MapChunkLoadedEvent : public UserInterface::Core::IEvent
    {
        ChunkCoordinate coordinate;
        std::vector<uint16_t> heightMap;
        std::vector<uint8_t> splatMap;

        /**
         * @brief Constructs the event.
         * @param coord The coordinates of the loaded chunk.
         */
        MapChunkLoadedEvent(ChunkCoordinate coord, std::vector<uint16_t> hm, std::vector<uint8_t> sm)
            : coordinate(coord), heightMap(std::move(hm)), splatMap(std::move(sm)) {}
    };

    struct TerrainCullCompletedEvent : public UserInterface::Core::IEvent
    {
        size_t visibleCount{0};
        explicit TerrainCullCompletedEvent(size_t count = 0) : visibleCount(count) {}
    };
} // namespace GameLib::Terrain::Events

namespace GameLib::Terrain {
    using Events::TerrainCullCompletedEvent;
    using Events::MapChunkLoadedEvent;
}

