#include "../StdAfx.h"
#include "IMapChunkStreamingService.h"

#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../../UserInterface/Core/EventBus.h"

#include <unordered_set>
#include <mutex>
#include <functional>
#include <cmath>

namespace GameLib::Terrain
{
    /**
     * @brief Zdarzenie wywolywane po wyczyszczeniu pamieci streamingu chunkow terenu.
     */
    struct MapChunksClearedEvent : public UserInterface::Core::IEvent
    {
        uint32_t clearedCount;

        /**
         * @brief Constructs the event.
         * @param count Number of chunks that were cleared.
         */
        explicit MapChunksClearedEvent(uint32_t count) : clearedCount(count) {}
    };

    class MapChunkStreamingService : public IMapChunkStreamingService
    {
    public:
        MapChunkStreamingService() = default;
        ~MapChunkStreamingService() override = default;

        void RequestChunk(ChunkCoordinate coord) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (loadedChunks_.insert(coord).second)
            {
                EterBase::ModernLogger::Debug("Chunk requested and loaded: ({}, {})", coord.sectorX, coord.sectorY);
            }
        }

        bool IsChunkLoaded(ChunkCoordinate coord) const override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return loadedChunks_.contains(coord);
        }

        void UpdateStreaming(float playerX, float playerY) override
        {
            EterBase::ModernLogger::Trace("Updating streaming at player pos: ({}, {})", playerX, playerY);
        }

        void EvictDistantChunks(float playerX, float playerY, float maxRadius) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto it = loadedChunks_.begin(); it != loadedChunks_.end(); )
            {
                float dx = static_cast<float>(it->sectorX) - playerX;
                float dy = static_cast<float>(it->sectorY) - playerY;
                float distanceSq = (dx * dx) + (dy * dy);

                if (distanceSq > (maxRadius * maxRadius))
                {
                    EterBase::ModernLogger::Debug("Evicting distant chunk: ({}, {})", it->sectorX, it->sectorY);
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
            std::lock_guard<std::mutex> lock(mutex_);
            uint32_t count = static_cast<uint32_t>(loadedChunks_.size());
            loadedChunks_.clear();
            
            EterBase::ModernLogger::Info("Cleared all map chunks. Total evicted: {}", count);
            
            UserInterface::Core::EventBus::GetInstance().Publish(MapChunksClearedEvent{count});
        }

    private:
        mutable std::mutex mutex_;
        std::unordered_set<ChunkCoordinate> loadedChunks_;
    };
}
