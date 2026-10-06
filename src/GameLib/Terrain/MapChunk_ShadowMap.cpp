#include "../StdAfx.h"
#include "IMapChunkStreamingService.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "UserInterface/Core/EventBus.h"

#include <unordered_set>
#include <cmath>
#include <expected>
#include <string_view>
#include <format>


namespace GameLib::Terrain::Events
{
    struct ShadowMapLoadedEvent
    {
        ChunkCoordinate coord;
    };

    struct ShadowMapEvictedEvent
    {
        ChunkCoordinate coord;
    };
}

namespace GameLib::Terrain
{
    class MapChunkShadowMapService final : public IMapChunkStreamingService
    {
    public:
        MapChunkShadowMapService() = default;
        ~MapChunkShadowMapService() override = default;

        void RequestChunk(ChunkCoordinate coord) override
        {
            if (IsChunkLoaded(coord))
            {
                return;
            }

            auto result = LoadShadowMap(coord);
            if (result.has_value())
            {
                loadedChunks_.insert(coord);
                EterBase::ModernLogger::Info("Successfully loaded shadow map for chunk [{}, {}]", coord.sectorX, coord.sectorY);
                UserInterface::Core::EventBus::GetInstance().Publish(Events::ShadowMapLoadedEvent{coord});
            }
            else
            {
                EterBase::ModernLogger::Error("Failed to load shadow map for chunk [{}, {}]: {}", coord.sectorX, coord.sectorY, result.error());
            }
        }

        bool IsChunkLoaded(ChunkCoordinate coord) const override
        {
            return loadedChunks_.contains(coord);
        }

        void UpdateStreaming(float playerX, float playerY) override
        {
            // Calculate current chunk based on player position (example mapping)
            int32_t currentSectorX = static_cast<int32_t>(playerX / 25600.0f);
            int32_t currentSectorY = static_cast<int32_t>(playerY / 25600.0f);
            
            ChunkCoordinate center{currentSectorX, currentSectorY};
            RequestChunk(center);
        }

        void EvictDistantChunks(float playerX, float playerY, float maxRadius) override
        {
            float maxRadiusSq = maxRadius * maxRadius;
            for (auto it = loadedChunks_.begin(); it != loadedChunks_.end(); )
            {
                float chunkWorldX = it->sectorX * 25600.0f;
                float chunkWorldY = it->sectorY * 25600.0f;
                
                float dx = playerX - chunkWorldX;
                float dy = playerY - chunkWorldY;
                
                if ((dx * dx + dy * dy) > maxRadiusSq)
                {
                    EterBase::ModernLogger::Info("Evicting shadow map for chunk [{}, {}]", it->sectorX, it->sectorY);
                    UserInterface::Core::EventBus::GetInstance().Publish(Events::ShadowMapEvictedEvent{*it});
                    it = loadedChunks_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void ClearAllChunks() override
        {
            for (const auto& coord : loadedChunks_)
            {
                UserInterface::Core::EventBus::GetInstance().Publish(Events::ShadowMapEvictedEvent{coord});
            }
            loadedChunks_.clear();
            EterBase::ModernLogger::Info("Cleared all shadow map chunks.");
        }

    private:
        std::expected<void, std::string_view> LoadShadowMap(ChunkCoordinate coord)
        {
            // Simulate shadow map loading logic. 
            // In a real implementation, this would read files and prepare GPU textures.
            if (coord.sectorX < -1000 || coord.sectorX > 1000 || coord.sectorY < -1000 || coord.sectorY > 1000)
            {
                return std::unexpected("Coordinate out of bounds.");
            }
            return {};
        }

        std::unordered_set<ChunkCoordinate> loadedChunks_;
    };
}
