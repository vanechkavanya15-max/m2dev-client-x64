#include "../StdAfx.h"
#include "ITerrainHeightCache.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../../UserInterface/Core/EventBus.h"

#include <cmath>
#include <vector>
#include <span>
#include <memory>

namespace UserInterface::Core {

    struct TerrainCacheInitializedEvent : public IEvent {
        uint32_t width;
        uint32_t height;
        
        TerrainCacheInitializedEvent(uint32_t w, uint32_t h) : width(w), height(h) {}
    };
    
    struct TerrainCacheClearedEvent : public IEvent {
        TerrainCacheClearedEvent() = default;
    };

} // namespace UserInterface::Core


namespace GameLib::Terrain {

    class TerrainHeightBilinearCache final : public ITerrainHeightCache {
    public:
        TerrainHeightBilinearCache(std::span<const uint16_t> heightmap, uint32_t width, uint32_t height, float cellSize, std::span<const uint8_t> watermap = {})
            : width_(width), height_(height), cellSize_(cellSize) {
            
            if (heightmap.empty() || width_ == 0 || height_ == 0 || cellSize_ <= 0.0f) {
                EterBase::ModernLogger::Error("TerrainHeightBilinearCache initialized with invalid dimensions or empty map.");
                return;
            }

            if (heightmap.size() < static_cast<size_t>(width_) * height_) {
                EterBase::ModernLogger::Error("TerrainHeightBilinearCache: Heightmap buffer too small.");
                return;
            }

            heightmap_.assign(heightmap.begin(), heightmap.end());
            
            if (!watermap.empty() && watermap.size() >= static_cast<size_t>(width_) * height_) {
                watermap_.assign(watermap.begin(), watermap.end());
            }

            EterBase::ModernLogger::Info("TerrainHeightBilinearCache initialized ({}x{}).", width_, height_);
            UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::TerrainCacheInitializedEvent(width_, height_));
        }

        float SampleHeight(float x, float y) const override {
            return TrySampleHeight(x, y, heightmap_).value_or(0.0f);
        }

        float SampleWaterHeight(float x, float y) const override {
            if (watermap_.empty()) return 0.0f;
            return TrySampleWaterHeight(x, y).value_or(0.0f);
        }

        bool IsWalkableSlope(float x, float y, float maxSlopeAngle) const override {
             return TryCalculateSlope(x, y).transform([maxSlopeAngle](float slope) {
                return slope <= maxSlopeAngle;
            }).value_or(false);
        }

        void BatchSampleHeight(const float* x, const float* y, float* outZ, size_t count) const override {
            if (!x || !y || !outZ || count == 0) return;
            for (size_t i = 0; i < count; ++i) {
                outZ[i] = SampleHeight(x[i], y[i]);
            }
        }

        void Clear() override {
            heightmap_.clear();
            watermap_.clear();
            width_ = 0;
            height_ = 0;
            EterBase::ModernLogger::Info("TerrainHeightBilinearCache cleared.");
            UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::TerrainCacheClearedEvent());
        }

    private:
        std::expected<float, EterBase::NavigationError> TrySampleHeight(float worldX, float worldY, std::span<const uint16_t> map) const {
            if (map.empty() || cellSize_ <= 0.0f || width_ == 0 || height_ == 0) {
                return std::unexpected(EterBase::NavigationError::MapNotLoaded);
            }

            float gridX = worldX / cellSize_;
            float gridY = worldY / cellSize_;

            if (gridX < 0.0f || gridY < 0.0f || 
                gridX >= static_cast<float>(width_ - 1) || 
                gridY >= static_cast<float>(height_ - 1)) {
                return std::unexpected(EterBase::NavigationError::DestinationUnreachable);
            }

            float xFloor = std::floor(gridX);
            float yFloor = std::floor(gridY);
            float xFraction = gridX - xFloor;
            float yFraction = gridY - yFloor;

            uint32_t x0 = static_cast<uint32_t>(xFloor);
            uint32_t y0 = static_cast<uint32_t>(yFloor);
            uint32_t x1 = x0 + 1;
            uint32_t y1 = y0 + 1;

            float h00 = static_cast<float>(map[y0 * width_ + x0]);
            float h10 = static_cast<float>(map[y0 * width_ + x1]);
            float h01 = static_cast<float>(map[y1 * width_ + x0]);
            float h11 = static_cast<float>(map[y1 * width_ + x1]);

            float h0 = std::lerp(h00, h10, xFraction);
            float h1 = std::lerp(h01, h11, xFraction);
            return std::lerp(h0, h1, yFraction);
        }

        std::expected<float, EterBase::NavigationError> TrySampleWaterHeight(float worldX, float worldY) const {
            if (watermap_.empty() || cellSize_ <= 0.0f || width_ == 0 || height_ == 0) {
                return std::unexpected(EterBase::NavigationError::MapNotLoaded);
            }

            float gridX = worldX / cellSize_;
            float gridY = worldY / cellSize_;

            if (gridX < 0.0f || gridY < 0.0f || 
                gridX >= static_cast<float>(width_ - 1) || 
                gridY >= static_cast<float>(height_ - 1)) {
                return std::unexpected(EterBase::NavigationError::DestinationUnreachable);
            }

            float xFloor = std::floor(gridX);
            float yFloor = std::floor(gridY);
            float xFraction = gridX - xFloor;
            float yFraction = gridY - yFloor;

            uint32_t x0 = static_cast<uint32_t>(xFloor);
            uint32_t y0 = static_cast<uint32_t>(yFloor);
            uint32_t x1 = x0 + 1;
            uint32_t y1 = y0 + 1;

            float h00 = static_cast<float>(watermap_[y0 * width_ + x0]);
            float h10 = static_cast<float>(watermap_[y0 * width_ + x1]);
            float h01 = static_cast<float>(watermap_[y1 * width_ + x0]);
            float h11 = static_cast<float>(watermap_[y1 * width_ + x1]);

            float h0 = std::lerp(h00, h10, xFraction);
            float h1 = std::lerp(h01, h11, xFraction);
            return std::lerp(h0, h1, yFraction);
        }

        std::expected<float, EterBase::NavigationError> TryCalculateSlope(float worldX, float worldY) const {
             return TrySampleHeight(worldX, worldY, heightmap_).and_then([&](float h) -> std::expected<float, EterBase::NavigationError> {
                
                auto hdx = TrySampleHeight(worldX + cellSize_, worldY, heightmap_);
                auto hdy = TrySampleHeight(worldX, worldY + cellSize_, heightmap_);

                if (!hdx.has_value() || !hdy.has_value()) {
                    return std::unexpected(EterBase::NavigationError::DestinationUnreachable);
                }

                float dx = hdx.value() - h;
                float dy = hdy.value() - h;

                float gradientLength = std::sqrt(dx * dx + dy * dy);
                float slopeRadians = std::atan2(gradientLength, cellSize_);
                
                return slopeRadians * (180.0f / 3.14159265358979323846f);
             });
        }


        std::vector<uint16_t> heightmap_;
        std::vector<uint8_t> watermap_;
        uint32_t width_ = 0;
        uint32_t height_ = 0;
        float cellSize_ = 0.0f;
    };


    std::unique_ptr<ITerrainHeightCache> CreateTerrainHeightBilinearCache(std::span<const uint16_t> heightmap, uint32_t width, uint32_t height, float cellSize, std::span<const uint8_t> watermap = {}) {
        return std::make_unique<TerrainHeightBilinearCache>(heightmap, width, height, cellSize, watermap);
    }

} // namespace GameLib::Terrain
