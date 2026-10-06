#include "../StdAfx.h"
#include "IMapChunkStreamingService.h"
#include "UserInterface/Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/AsyncTaskRunnerLite.h"
#include "EterBase/Result.h"

#include <unordered_set>
#include <mutex>
#include <memory>
#include <cmath>
#include <vector>


namespace GameLib::Terrain
{
    // Custom events defined here to adhere to Zero-Conflict rule
    struct ChunkLoadedEvent : public UserInterface::Core::IEvent
    {
        ChunkCoordinate coord;
        explicit ChunkLoadedEvent(ChunkCoordinate c) : coord(c) {}
    };

    struct ChunkEvictedEvent : public UserInterface::Core::IEvent
    {
        ChunkCoordinate coord;
        explicit ChunkEvictedEvent(ChunkCoordinate c) : coord(c) {}
    };

    class MapChunkStreamingService : public IMapChunkStreamingService
    {
    public:
        MapChunkStreamingService() : m_taskRunner(2)
        {
            EterBase::ModernLogger::Info("MapChunkStreamingService initialized.");
        }

        ~MapChunkStreamingService() override
        {
            ClearAllChunks();
        }

        void RequestChunk(ChunkCoordinate coord) override
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_loadedChunks.contains(coord) || m_loadingChunks.contains(coord))
                {
                    return; // Already loaded or currently loading
                }
                m_loadingChunks.insert(coord);
            }

            EterBase::ModernLogger::Debug("Requesting async load for chunk [{}, {}]", coord.sectorX, coord.sectorY);

            // Submit background task
            m_taskRunner.SubmitTask(std::nullopt, [this, coord]() -> EterBase::VoidResult<> {
                // Simulate loading delay/work
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_loadingChunks.erase(coord);
                    m_loadedChunks.insert(coord);
                }

                EterBase::ModernLogger::Info("Chunk [{}, {}] loaded asynchronously.", coord.sectorX, coord.sectorY);

                // Notify systems that a chunk was successfully loaded
                UserInterface::Core::EventBus::GetInstance().Publish(ChunkLoadedEvent{coord});

                return {};
            });
        }

        bool IsChunkLoaded(ChunkCoordinate coord) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_loadedChunks.contains(coord);
        }

        void UpdateStreaming(float /*playerX*/, float /*playerY*/) override
        {
            // Empty update for now. Streaming updates might trigger RequestChunk internally 
            // if we implemented a grid-based pre-fetch logic here.
        }

        void EvictDistantChunks(float playerX, float playerY, float maxRadius) override
        {
            std::vector<ChunkCoordinate> evictedChunks;

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                
                for (auto it = m_loadedChunks.begin(); it != m_loadedChunks.end(); )
                {
                    // Simple distance calculation (assuming 1 sector = roughly some scale, here just comparing directly)
                    float dx = static_cast<float>(it->sectorX) - playerX;
                    float dy = static_cast<float>(it->sectorY) - playerY;
                    float distanceSq = (dx * dx) + (dy * dy);
                    
                    if (distanceSq > (maxRadius * maxRadius))
                    {
                        ChunkCoordinate evictedCoord = *it;
                        evictedChunks.push_back(evictedCoord);
                        it = m_loadedChunks.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }

            // Publish events outside of the lock to prevent deadlocks
            for (const auto& evictedCoord : evictedChunks)
            {
                EterBase::ModernLogger::Info("Evicting chunk [{}, {}] due to distance.", evictedCoord.sectorX, evictedCoord.sectorY);
                UserInterface::Core::EventBus::GetInstance().Publish(ChunkEvictedEvent{evictedCoord});
            }
        }

        void ClearAllChunks() override
        {
            std::vector<ChunkCoordinate> evictedChunks;

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                for (const auto& coord : m_loadedChunks)
                {
                    evictedChunks.push_back(coord);
                }
                m_loadedChunks.clear();
                m_loadingChunks.clear();
            }

            // Publish events outside of the lock to prevent deadlocks
            for (const auto& coord : evictedChunks)
            {
                UserInterface::Core::EventBus::GetInstance().Publish(ChunkEvictedEvent{coord});
            }

            EterBase::ModernLogger::Info("All chunks cleared.");
        }

    private:
        mutable std::mutex m_mutex;
        std::unordered_set<ChunkCoordinate> m_loadedChunks;
        std::unordered_set<ChunkCoordinate> m_loadingChunks;
        EterBase::AsyncTaskRunnerLite m_taskRunner;
    };

    // Factory function to expose the service
    std::unique_ptr<IMapChunkStreamingService> CreateMapChunkStreamingService()
    {
        return std::make_unique<MapChunkStreamingService>();
    }
}
