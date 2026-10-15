#include "../StdAfx.h"
#include "ChunkStreamingCoordinator.h"
#include "TerrainEvents.h"
#include "../../EterBase/LogModern.h"
#include <algorithm>
#include <cmath>

namespace GameLib::Terrain {

    std::unique_ptr<IMapChunkStreamingService> CreateMapChunkStreamingService(TerrainHeightStorage* heightStorage) {
        return std::make_unique<ChunkStreamingCoordinator>(heightStorage);
    }

    ChunkStreamingCoordinator::ChunkStreamingCoordinator(TerrainHeightStorage* heightStorage)
        : m_heightStorage(heightStorage) {}

    void ChunkStreamingCoordinator::RequestChunk(ChunkCoordinate coord) {
        std::lock_guard lock(m_mutex);
        SectorCoord sc = coord;
        auto& slot = m_chunks[sc];
        slot.coord = sc;
        if (slot.state == ChunkState::Unloaded) {
            slot.state = ChunkState::Queued;
            slot.loadPriority = 1.0f;
            EterBase::ModernLogger::Debug("Chunk [{}, {}] manually queued.", sc.x, sc.y);
        }
    }

    bool ChunkStreamingCoordinator::IsChunkLoaded(ChunkCoordinate coord) const {
        std::lock_guard lock(m_mutex);
        SectorCoord sc = coord;
        auto it = m_chunks.find(sc);
        return it != m_chunks.end() && it->second.state == ChunkState::Active;
    }

    void ChunkStreamingCoordinator::MarkChunkActive(SectorCoord coord) {
        std::lock_guard lock(m_mutex);
        auto& slot = m_chunks[coord];
        slot.coord = coord;
        slot.state = ChunkState::Active;
        EterBase::ModernLogger::Info("Chunk [{}, {}] marked ACTIVE.", coord.x, coord.y);
    }

    ChunkState ChunkStreamingCoordinator::GetChunkState(SectorCoord coord) const noexcept {
        std::lock_guard lock(m_mutex);
        auto it = m_chunks.find(coord);
        return (it != m_chunks.end()) ? it->second.state : ChunkState::Unloaded;
    }

    std::vector<SectorCoord> ChunkStreamingCoordinator::GetPendingLoadQueue() {
        std::lock_guard lock(m_mutex);
        std::vector<std::pair<float, SectorCoord>> pending;

        for (const auto& [coord, slot] : m_chunks) {
            if (slot.state == ChunkState::Queued) {
                pending.emplace_back(slot.loadPriority, coord);
            }
        }

        // Sortowanie malejaco po priorytecie
        std::sort(pending.begin(), pending.end(), [](const auto& a, const auto& b) {
            return a.first > b.first;
        });

        std::vector<SectorCoord> result;
        result.reserve(pending.size());
        for (const auto& item : pending) {
            result.push_back(item.second);
        }
        return result;
    }

    void ChunkStreamingCoordinator::UpdateStreaming(float playerX, float playerY) {
        WorldPosition playerPos{playerX, playerY, 0.0f};
        WorldPosition playerVel{0.0f, 0.0f, 0.0f};

        if (m_hasLastPos) {
            playerVel.x = playerPos.x - m_lastPlayerPos.x;
            playerVel.y = playerPos.y - m_lastPlayerPos.y;
        } else {
            m_hasLastPos = true;
        }
        m_lastPlayerPos = playerPos;

        // Domyslny promien strumieniowania: 1.5 sektora (38400 jednostek)
        constexpr float viewRadius = TerrainMetrics::SectorSize * 1.5f;
        constexpr float viewRadiusSq = viewRadius * viewRadius;

        SectorCoord centerSector = TerrainMetrics::WorldToSector(playerPos);
        int32_t sectorRadius = static_cast<int32_t>(std::ceil(viewRadius / TerrainMetrics::SectorSize));

        std::lock_guard lock(m_mutex);

        // 1. Zidentyfikuj potrzebne sektory w zasiegu widzenia
        for (int32_t dy = -sectorRadius; dy <= sectorRadius; ++dy) {
            for (int32_t dx = -sectorRadius; dx <= sectorRadius; ++dx) {
                SectorCoord target{centerSector.x + dx, centerSector.y + dy};

                // Bezpieczny pomiar dystansu: Odleglosc do prostokata AABB sektora!
                // Dla sektora gracza distSq wynosi dokladnie 0.0f!
                float distSq = TerrainMetrics::DistanceSqPointToSectorAABB(playerPos, target);

                if (distSq <= viewRadiusSq) {
                    EnsureSlot(target, playerPos, playerVel);
                }
            }
        }

        // 2. Bezpieczna ewikcja z histereza (+2000 jednostek bufora przeciwko migotaniu krawedzi)
        constexpr float evictRadius = viewRadius + 2000.0f;
        constexpr float evictRadiusSq = evictRadius * evictRadius;

        std::vector<SectorCoord> toEvict;
        for (const auto& [coord, slot] : m_chunks) {
            float distSq = TerrainMetrics::DistanceSqPointToSectorAABB(playerPos, coord);
            if (distSq > evictRadiusSq) {
                toEvict.push_back(coord);
            }
        }

        for (const auto& coord : toEvict) {
            auto it = m_chunks.find(coord);
            if (it != m_chunks.end()) {
                if (it->second.state == ChunkState::Active && m_heightStorage) {
                    m_heightStorage->RemoveChunk(coord);
                }
                m_chunks.erase(it);
                UserInterface::Core::EventBus::GetInstance().Publish(ChunkEvictedEvent{coord});
                EterBase::ModernLogger::Debug("Chunk [{}, {}] evicted safely.", coord.x, coord.y);
            }
        }
    }

    void ChunkStreamingCoordinator::EvictDistantChunks(float playerX, float playerY, float maxRadius) {
        WorldPosition playerPos{playerX, playerY, 0.0f};
        float maxRadiusSq = maxRadius * maxRadius;

        std::lock_guard lock(m_mutex);
        std::vector<SectorCoord> toEvict;

        for (const auto& [coord, slot] : m_chunks) {
            float distSq = TerrainMetrics::DistanceSqPointToSectorAABB(playerPos, coord);
            if (distSq > maxRadiusSq) {
                toEvict.push_back(coord);
            }
        }

        for (const auto& coord : toEvict) {
            auto it = m_chunks.find(coord);
            if (it != m_chunks.end()) {
                if (it->second.state == ChunkState::Active && m_heightStorage) {
                    m_heightStorage->RemoveChunk(coord);
                }
                m_chunks.erase(it);
                UserInterface::Core::EventBus::GetInstance().Publish(ChunkEvictedEvent{coord});
            }
        }
    }

    void ChunkStreamingCoordinator::ClearAllChunks() {
        std::lock_guard lock(m_mutex);
        for (const auto& [coord, slot] : m_chunks) {
            UserInterface::Core::EventBus::GetInstance().Publish(ChunkEvictedEvent{coord});
        }
        m_chunks.clear();
        m_hasLastPos = false;
        if (m_heightStorage) {
            m_heightStorage->Clear();
        }
        EterBase::ModernLogger::Info("ChunkStreamingCoordinator: All chunks cleared.");
    }

    void ChunkStreamingCoordinator::EnsureSlot(SectorCoord coord, WorldPosition playerPos, WorldPosition playerVel) {
        auto& slot = m_chunks[coord];
        slot.coord = coord;

        if (slot.state == ChunkState::Unloaded) {
            slot.state = ChunkState::Queued;

            // Obliczenie priorytetu ladowania
            WorldPosition targetCenter = TerrainMetrics::SectorToWorldCenter(coord);
            float toTargetX = targetCenter.x - playerPos.x;
            float toTargetY = targetCenter.y - playerPos.y;
            float centerDist = std::max(std::sqrt(toTargetX * toTargetX + toTargetY * toTargetY), 1.0f);

            float dot = 0.0f;
            float velLenSq = playerVel.x * playerVel.x + playerVel.y * playerVel.y;
            if (velLenSq > 0.01f) {
                float velLen = std::sqrt(velLenSq);
                dot = (playerVel.x / velLen) * (toTargetX / centerDist) +
                      (playerVel.y / velLen) * (toTargetY / centerDist);
            }

            // Bliskosc + wspolczynnik kierunkowy wektora ruchu
            slot.loadPriority = (1.0f / centerDist) * (1.0f + std::max(0.0f, dot));
        }
    }

} // namespace GameLib::Terrain
