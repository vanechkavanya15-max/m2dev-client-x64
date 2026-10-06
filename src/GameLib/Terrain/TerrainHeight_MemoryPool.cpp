#include "../StdAfx.h"
#include "ITerrainHeightCache.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../../UserInterface/Core/EventBus.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <limits>

namespace GameLib::Terrain
{
    struct TerrainCacheClearedEvent : public UserInterface::Core::IEvent {};

    class TerrainHeightMemoryPool final : public ITerrainHeightCache
    {
    public:
        TerrainHeightMemoryPool()
            : m_heightCache(std::make_unique<CacheArray>()),
              m_waterCache(std::make_unique<CacheArray>())
        {
            EterBase::ModernLogger::Info("TerrainHeightMemoryPool initialized with static grid {}", POOL_GRID_SIZE);
            Clear();
        }

        ~TerrainHeightMemoryPool() override = default;

        /**
         * @brief Samples the terrain height at the given world coordinates.
         * @param x World X coordinate.
         * @param y World Y coordinate.
         * @return The terrain height (Z coordinate).
         */
        float SampleHeight(float x, float y) const override
        {
            const uint32_t index = CalculateIndex(x, y);
            auto& entry = m_heightCache->at(index);

            if (entry.isCached && std::abs(entry.x - x) < 0.1f && std::abs(entry.y - y) < 0.1f) {
                return entry.height;
            }

            // In a real scenario, this would call the underlying map data.
            // Since ITerrainHeightCache interface dictates caching, we assume 
            // the actual terrain interpolation logic happens here if not cached.
            // As we are only writing the pool infrastructure, we return a fallback.
            
            float height = 0.0f; // Placeholder: Actual logic would sample the height map
            
            entry = { x, y, height, true };
            return height;
        }

        /**
         * @brief Samples the water height at the given world coordinates.
         * @param x World X coordinate.
         * @param y World Y coordinate.
         * @return The water height (Z coordinate).
         */
        float SampleWaterHeight(float x, float y) const override
        {
            const uint32_t index = CalculateIndex(x, y);
            auto& entry = m_waterCache->at(index);

            if (entry.isCached && std::abs(entry.x - x) < 0.1f && std::abs(entry.y - y) < 0.1f) {
                return entry.height;
            }

            float waterHeight = 0.0f; // Placeholder for actual map sampling

            entry = { x, y, waterHeight, true };
            return waterHeight;
        }

        /**
         * @brief Checks if the slope at (x, y) is walkable given a maximum angle.
         * @param x World X coordinate.
         * @param y World Y coordinate.
         * @param maxSlopeAngle Maximum allowed slope angle in degrees.
         * @return True if the slope is walkable, false otherwise.
         */
        bool IsWalkableSlope(float x, float y, float maxSlopeAngle) const override
        {
            // Simplified gradient calculation using a small epsilon step
            const float epsilon = 50.0f; 
            const float hCenter = SampleHeight(x, y);
            const float hRight = SampleHeight(x + epsilon, y);
            const float hTop = SampleHeight(x, y + epsilon);

            const float dzdx = (hRight - hCenter) / epsilon;
            const float dzdy = (hTop - hCenter) / epsilon;
            
            // Maximum gradient
            const float maxGradient = std::max(std::abs(dzdx), std::abs(dzdy));
            const float slopeAngle = std::atan(maxGradient) * (180.0f / 3.14159265f);
            
            return slopeAngle <= maxSlopeAngle;
        }

        /**
         * @brief Batches multiple height samples into an output array.
         * @param x Array of X coordinates.
         * @param y Array of Y coordinates.
         * @param outZ Output array for heights.
         * @param count Number of samples.
         */
        void BatchSampleHeight(const float* x, const float* y, float* outZ, size_t count) const override
        {
            for (size_t i = 0; i < count; ++i) {
                outZ[i] = SampleHeight(x[i], y[i]);
            }
        }

        /**
         * @brief Clears the memory pool caches.
         */
        void Clear() override
        {
            for (auto& entry : *m_heightCache) {
                entry.isCached = false;
            }
            for (auto& entry : *m_waterCache) {
                entry.isCached = false;
            }
            
            EterBase::ModernLogger::Debug("TerrainHeightMemoryPool cache cleared");
            
            UserInterface::Core::EventBus::GetInstance().Publish(TerrainCacheClearedEvent{});
        }

    private:
        struct CacheEntry {
            float x;
            float y;
            float height;
            bool isCached;
        };

        // Static grid configuration for memory pool mapping
        static constexpr size_t POOL_GRID_SIZE = 1024;
        static constexpr size_t POOL_TOTAL_ENTRIES = POOL_GRID_SIZE * POOL_GRID_SIZE;
        using CacheArray = std::array<CacheEntry, POOL_TOTAL_ENTRIES>;

        mutable std::unique_ptr<CacheArray> m_heightCache;
        mutable std::unique_ptr<CacheArray> m_waterCache;

        /**
         * @brief Calculates a 1D spatial hash index for the 2D coordinate.
         * @param x World X coordinate.
         * @param y World Y coordinate.
         * @return The cache array index.
         */
        [[nodiscard]] uint32_t CalculateIndex(float x, float y) const
        {
            if (!std::isfinite(x) || !std::isfinite(y)) {
                return 0; // Fallback index for NaN / Infinity
            }

            // Simple spatial hashing into the grid
            int32_t ix = static_cast<int32_t>(std::round(x / 100.0f)) % static_cast<int32_t>(POOL_GRID_SIZE);
            int32_t iy = static_cast<int32_t>(std::round(y / 100.0f)) % static_cast<int32_t>(POOL_GRID_SIZE);
            
            if (ix < 0) ix += POOL_GRID_SIZE;
            if (iy < 0) iy += POOL_GRID_SIZE;

            return static_cast<uint32_t>(iy * POOL_GRID_SIZE + ix);
        }
    };

    /**
     * @brief Factory function to create a new TerrainHeightMemoryPool instance.
     * @return A standard expected wrapping a unique_ptr to the memory pool interface or an error.
     */
    std::expected<std::unique_ptr<ITerrainHeightCache>, EterBase::EntityError> CreateTerrainHeightMemoryPool()
    {
        try {
            return std::make_unique<TerrainHeightMemoryPool>();
        } catch (...) {
            EterBase::ModernLogger::Error("Failed to create TerrainHeightMemoryPool");
            return std::unexpected(EterBase::EntityError::NotFound);
        }
    }
}
