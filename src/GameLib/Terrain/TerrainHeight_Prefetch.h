#pragma once

#include "ITerrainHeightCache.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../UserInterface/Core/EventBus.h"

#include <span>
#include <vector>

namespace GameLib::Terrain
{
    /**
     * @brief Zdarzenie wyzwalane po zakonczeniu pre-fetchingu kafelkow wysokosci do cache.
     */
    struct TerrainHeightPrefetchedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId entityId;
        size_t count;

        TerrainHeightPrefetchedEvent(EterBase::EntityId entityId, size_t count)
            : entityId(entityId), count(count) {}
    };

    /**
     * @brief Klasa odpowiedzialna za pre-fetching wysokosci terenu z wyprzedzeniem przed ruchem, 
     * aby uniknac cache miss w glownej petli kolizji.
     */
    class TerrainHeightPrefetcher
    {
    public:
        TerrainHeightPrefetcher(ITerrainHeightCache* heightCache);
        ~TerrainHeightPrefetcher() = default;

        /**
         * @brief Prefetch heights for given positions
         * @param entityId The entity ID requesting the prefetch
         * @param xPositions Array of X coordinates to prefetch
         * @param yPositions Array of Y coordinates to prefetch
         * @return PacketResult<void> Returns success or error on invalid input
         */
        EterBase::PacketResult<void> PrefetchAhead(
            EterBase::EntityId entityId,
            std::span<const float> xPositions,
            std::span<const float> yPositions);

    private:
        ITerrainHeightCache* heightCache;
    };
}
