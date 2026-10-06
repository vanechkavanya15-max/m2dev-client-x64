#include "../StdAfx.h"
#include "IMapChunkStreamingService.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/LogModern.h"
#include "../../UserInterface/Core/EventBus.h"

#include <unordered_set>
#include <queue>
#include <vector>
#include <cmath>
#include <functional>
#include <algorithm>
#include <optional>

namespace GameLib::Terrain
{
    // C++23 hash function for ChunkCoordinate to use in unordered_set
    struct ChunkCoordinateHash {
        std::size_t operator()(const ChunkCoordinate& coord) const noexcept {
            return std::hash<int32_t>{}(coord.sectorX) ^ (std::hash<int32_t>{}(coord.sectorY) << 1);
        }
    };

    struct ChunkCoordinateEqual {
        bool operator()(const ChunkCoordinate& a, const ChunkCoordinate& b) const noexcept {
            return a.sectorX == b.sectorX && a.sectorY == b.sectorY;
        }
    };

    /**
     * @brief Zdarzenie emitowane gdy chunk zostanie zaladowany.
     */
    struct ChunkLoadedEvent : public UserInterface::Core::IEvent {
        ChunkCoordinate coord;
        explicit ChunkLoadedEvent(ChunkCoordinate c) : coord(c) {}
    };

    /**
     * @brief Zdarzenie emitowane gdy chunk zostanie usuniety (evicted).
     */
    struct ChunkEvictedEvent : public UserInterface::Core::IEvent {
        ChunkCoordinate coord;
        explicit ChunkEvictedEvent(ChunkCoordinate c) : coord(c) {}
    };

    namespace 
    {
        class MapChunkPriorityQueue final : public IMapChunkStreamingService
        {
        public:
            MapChunkPriorityQueue() = default;
            ~MapChunkPriorityQueue() override = default;

            void RequestChunk(ChunkCoordinate coord) override
            {
                if (IsChunkLoaded(coord)) {
                    return;
                }

                auto it = std::find_if(m_pendingRequests.begin(), m_pendingRequests.end(),
                    [&coord](const ChunkCoordinate& c) {
                        return c.sectorX == coord.sectorX && c.sectorY == coord.sectorY;
                    });

                if (it == m_pendingRequests.end()) {
                    m_pendingRequests.push_back(coord);
                    EterBase::ModernLogger::Debug("Chunk requested: ({}, {})", coord.sectorX, coord.sectorY);
                }
            }

            bool IsChunkLoaded(ChunkCoordinate coord) const override
            {
                return m_loadedChunks.contains(coord);
            }

            void UpdateStreaming(float playerX, float playerY) override
            {
                if (!m_initializedPosition) {
                    m_lastPlayerX = playerX;
                    m_lastPlayerY = playerY;
                    m_initializedPosition = true;
                }

                float dx = playerX - m_lastPlayerX;
                float dy = playerY - m_lastPlayerY;
                float lenSq = dx * dx + dy * dy;

                float dirX = 0.0f;
                float dirY = 0.0f;

                if (lenSq > 0.001f) {
                    float len = std::sqrt(lenSq);
                    dirX = dx / len;
                    dirY = dy / len;
                }

                m_lastPlayerX = playerX;
                m_lastPlayerY = playerY;

                auto nextChunkResult = GetNextChunkToLoad(playerX, playerY, dirX, dirY);
                if (nextChunkResult.has_value()) {
                    ChunkCoordinate toLoad = nextChunkResult.value();
                    
                    auto it = std::find_if(m_pendingRequests.begin(), m_pendingRequests.end(),
                        [&toLoad](const ChunkCoordinate& c) {
                            return c.sectorX == toLoad.sectorX && c.sectorY == toLoad.sectorY;
                        });
                    
                    if (it != m_pendingRequests.end()) {
                        m_pendingRequests.erase(it);
                    }

                    m_loadedChunks.insert(toLoad);
                    EterBase::ModernLogger::Info("Chunk loaded: ({}, {})", toLoad.sectorX, toLoad.sectorY);

                    UserInterface::Core::EventBus::GetInstance().Publish(ChunkLoadedEvent{toLoad});
                }
            }

            void EvictDistantChunks(float playerX, float playerY, float maxRadius) override
            {
                float maxRadiusSq = maxRadius * maxRadius;
                for (auto it = m_loadedChunks.begin(); it != m_loadedChunks.end(); ) {
                    float dx = static_cast<float>(it->sectorX) - playerX;
                    float dy = static_cast<float>(it->sectorY) - playerY;
                    float distSq = dx * dx + dy * dy;

                    if (distSq > maxRadiusSq) {
                        EterBase::ModernLogger::Info("Chunk evicted: ({}, {})", it->sectorX, it->sectorY);
                        UserInterface::Core::EventBus::GetInstance().Publish(ChunkEvictedEvent{*it});
                        it = m_loadedChunks.erase(it);
                    } else {
                        ++it;
                    }
                }
            }

            void ClearAllChunks() override
            {
                for (const auto& chunk : m_loadedChunks) {
                    UserInterface::Core::EventBus::GetInstance().Publish(ChunkEvictedEvent{chunk});
                }
                m_loadedChunks.clear();
                m_pendingRequests.clear();
                m_initializedPosition = false;
                EterBase::ModernLogger::Info("All chunks cleared");
            }

        private:
            // Oblicza wynik (score) dla chunk'a w oparciu o dystans i kierunek kamery
            float CalculateChunkPriorityScore(const ChunkCoordinate& c, float playerX, float playerY, float dirX, float dirY) const
            {
                float cx = static_cast<float>(c.sectorX) - playerX;
                float cy = static_cast<float>(c.sectorY) - playerY;
                float distSq = cx * cx + cy * cy;
                
                // Bazowy wynik: im blizej, tym wyzszy priorytet (mniejsza odleglosc -> mniejszy wynik bazowy)
                float score = distSq;

                if (distSq > 0.001f && (dirX != 0.0f || dirY != 0.0f)) {
                    float dist = std::sqrt(distSq);
                    float normX = cx / dist;
                    float normY = cy / dist;
                    
                    // Dot product: od -1 (za nami) do 1 (przed nami)
                    float dotProduct = normX * dirX + normY * dirY;
                    
                    // Jesli chunk jest przed nami, zwieksz jego priorytet poprzez ZMNIEJSZENIE jego score (bo sortujemy malejaco po mniejszym score)
                    // Np. o polowe dystansu, gdy jest dokladnie na wprost.
                    if (dotProduct > 0.0f) {
                        score -= distSq * dotProduct * 0.5f; 
                    } else {
                        // Chunk z tylu - karzemy zwiekszajac score.
                        score += distSq * std::abs(dotProduct) * 0.5f;
                    }
                }
                return score;
            }

            EterBase::Result<ChunkCoordinate, EterBase::NavigationError> GetNextChunkToLoad(float playerX, float playerY, float dirX, float dirY) 
            {
                if (m_pendingRequests.empty()) {
                    return std::unexpected(EterBase::NavigationError::PathNotFound);
                }

                auto bestIt = m_pendingRequests.begin();
                float bestScore = CalculateChunkPriorityScore(*bestIt, playerX, playerY, dirX, dirY);

                for (auto it = m_pendingRequests.begin() + 1; it != m_pendingRequests.end(); ++it) {
                    float score = CalculateChunkPriorityScore(*it, playerX, playerY, dirX, dirY);
                    if (score < bestScore) {
                        bestScore = score;
                        bestIt = it;
                    }
                }

                return *bestIt;
            }

            std::unordered_set<ChunkCoordinate, ChunkCoordinateHash, ChunkCoordinateEqual> m_loadedChunks;
            std::vector<ChunkCoordinate> m_pendingRequests;

            float m_lastPlayerX{0.0f};
            float m_lastPlayerY{0.0f};
            bool m_initializedPosition{false};
        };
    } // namespace anonymous

    // Factory function if needed to instantiate this service
    std::unique_ptr<IMapChunkStreamingService> CreateMapChunkStreamingService() {
        return std::make_unique<MapChunkPriorityQueue>();
    }

} // namespace GameLib::Terrain
