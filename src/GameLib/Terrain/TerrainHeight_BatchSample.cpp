#include "../StdAfx.h"
#include "ITerrainHeightCache.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "UserInterface/Core/EventBus.h"

#include <span>
#include <array>
#include <cstdint>
#include <cstddef>
#include <expected>

namespace GameLib::Terrain
{
    /**
     * @brief Event published when a batch of entity terrain heights is sampled.
     */
    struct TerrainHeightBatchSampledEvent final : public UserInterface::Core::IEvent
    {
        std::array<EterBase::EntityId, 8> entityIds{};
        std::array<float, 8> heights{};
        std::size_t count = 0;
    };

    /**
     * @brief Handler for batch sampling terrain heights.
     */
    class TerrainHeightBatchSampler
    {
    public:
        /**
         * @brief Samples terrain height for up to 8 entities without dynamic allocations.
         * 
         * @param cache The terrain height cache interface.
         * @param entityIds Span of entity IDs (exactly 8 elements).
         * @param xs Span of X coordinates (exactly 8 elements).
         * @param ys Span of Y coordinates (exactly 8 elements).
         * @return std::expected<void, EterBase::EntityError> success or error on dimension mismatch.
         */
        static std::expected<void, EterBase::EntityError> SampleBatch(
            const ITerrainHeightCache& cache,
            std::span<const EterBase::EntityId> entityIds,
            std::span<const float> xs,
            std::span<const float> ys)
        {
            constexpr std::size_t BatchSize = 8;
            
            if (entityIds.size() != BatchSize || xs.size() != BatchSize || ys.size() != BatchSize)
            {
                EterBase::ModernLogger::Error("TerrainHeightBatchSampler: Invalid span sizes for batch sampling.");
                return std::unexpected(EterBase::EntityError::OutOfRange);
            }

            EterBase::ModernLogger::Debug("TerrainHeightBatchSampler: Processing batch sample for 8 entities.");

            std::array<float, BatchSize> zs{};
            cache.BatchSampleHeight(xs.data(), ys.data(), zs.data(), BatchSize);

            TerrainHeightBatchSampledEvent event;
            for (std::size_t i = 0; i < BatchSize; ++i)
            {
                event.entityIds[i] = entityIds[i];
                event.heights[i] = zs[i];
            }
            event.count = BatchSize;

            UserInterface::Core::EventBus::GetInstance().Publish(event);

            EterBase::ModernLogger::Info("TerrainHeightBatchSampler: Successfully sampled and broadcasted heights for 8 entities.");

            return {};
        }
    };
}
