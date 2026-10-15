#include "../StdAfx.h"
#include "TerrainHeight_Prefetch.h"
#include "../../EterBase/LogModern.h"
#include <vector>

namespace GameLib::Terrain
{
    TerrainHeightPrefetcher::TerrainHeightPrefetcher(ITerrainHeightCache* heightCache)
        : heightCache(heightCache)
    {
    }

    EterBase::PacketResult<void> TerrainHeightPrefetcher::PrefetchAhead(
        EterBase::EntityId entityId,
        std::span<const float> xPositions,
        std::span<const float> yPositions)
    {
        if (!heightCache)
        {
            EterBase::ModernLogger::Error("TerrainHeightPrefetcher::PrefetchAhead - heightCache is null");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        if (xPositions.size() != yPositions.size())
        {
            EterBase::ModernLogger::Error("TerrainHeightPrefetcher::PrefetchAhead - xPositions and yPositions size mismatch");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        const size_t count = xPositions.size();

        if (count == 0)
        {
            return {};
        }

        // Temporary buffer to hold the prefetched heights
        std::vector<float> prefetchedZ(count);

        // Perform the batch sample to prefetch into L2/L3 cache
        heightCache->BatchSampleHeight(xPositions.data(), yPositions.data(), prefetchedZ.data(), count);

        // Publish event indicating we've prefetched
        UserInterface::Core::EventBus::GetInstance().Publish(
            TerrainHeightPrefetchedEvent(entityId, count)
        );

        EterBase::ModernLogger::Debug("TerrainHeightPrefetcher::PrefetchAhead - Prefetched {} heights for entity {}", count, entityId.value());

        return {};
    }
}
