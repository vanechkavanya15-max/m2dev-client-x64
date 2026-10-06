#include "../StdAfx.h"
#include "IMapChunkStreamingService.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../../UserInterface/Core/EventBus.h"
#include <unordered_set>
#include <cmath>

namespace GameLib::Terrain {

/**
 * @brief Zdarzenie zadania zaladowania sektora mapy przez systemy zalezne (np. Cache/IO).
 */
struct ChunkLoadRequestEvent : public UserInterface::Core::IEvent {
    ChunkCoordinate coord;
    explicit ChunkLoadRequestEvent(ChunkCoordinate c) : coord(c) {}
};

/**
 * @brief Zdarzenie zadania odaladowania sektora mapy z pamieci.
 */
struct ChunkUnloadRequestEvent : public UserInterface::Core::IEvent {
    ChunkCoordinate coord;
    explicit ChunkUnloadRequestEvent(ChunkCoordinate c) : coord(c) {}
};

struct ChunkCoordinateHash {
    std::size_t operator()(const ChunkCoordinate& c) const noexcept {
        return std::hash<int32_t>{}(c.sectorX) ^ (std::hash<int32_t>{}(c.sectorY) << 1);
    }
};

struct ChunkCoordinateEqual {
    bool operator()(const ChunkCoordinate& a, const ChunkCoordinate& b) const noexcept {
        return a.sectorX == b.sectorX && a.sectorY == b.sectorY;
    }
};

/**
 * @brief Implementacja serwisu strumieniowania sektorow mapy oparta o SRP i szyny zdarzen.
 */
class MapChunkStreamingService : public IMapChunkStreamingService {
public:
    static constexpr float SECTOR_SIZE = 25600.0f;

    MapChunkStreamingService() {
        EterBase::ModernLogger::Info("MapChunkStreamingService initialized.");
    }

    ~MapChunkStreamingService() override {
        ClearAllChunks();
        EterBase::ModernLogger::Info("MapChunkStreamingService destroyed.");
    }

    void RequestChunk(ChunkCoordinate coord) override {
        if (IsChunkLoaded(coord)) {
            return;
        }

        EterBase::ModernLogger::Debug("Requesting map chunk load at sector ({}, {})", coord.sectorX, coord.sectorY);
        m_activeChunks.insert(coord);

        UserInterface::Core::EventBus::GetInstance().Publish(ChunkLoadRequestEvent{coord});
    }

    [[nodiscard]] bool IsChunkLoaded(ChunkCoordinate coord) const override {
        return m_activeChunks.contains(coord);
    }

    void UpdateStreaming(float playerX, float playerY) override {
        auto result = CalculateSector(playerX, playerY);
        if (!result.has_value()) {
            EterBase::ModernLogger::Error("Failed to calculate sector for streaming update at ({}, {})", playerX, playerY);
            return;
        }

        ChunkCoordinate currentSector = result.value();
        
        // Zglasza zadanie wczytania srodkowego sektora i sasiadow (3x3 grid)
        for (int32_t dx = -1; dx <= 1; ++dx) {
            for (int32_t dy = -1; dy <= 1; ++dy) {
                RequestChunk(ChunkCoordinate{currentSector.sectorX + dx, currentSector.sectorY + dy});
            }
        }
    }

    void EvictDistantChunks(float playerX, float playerY, float maxRadius) override {
        auto result = CalculateSector(playerX, playerY);
        if (!result) return;

        ChunkCoordinate center = result.value();
        float maxRadiusSectors = maxRadius / SECTOR_SIZE;
        float maxDistSq = maxRadiusSectors * maxRadiusSectors;

        for (auto it = m_activeChunks.begin(); it != m_activeChunks.end(); ) {
            float dx = static_cast<float>(it->sectorX - center.sectorX);
            float dy = static_cast<float>(it->sectorY - center.sectorY);
            float distSq = dx * dx + dy * dy;

            if (distSq > maxDistSq) {
                EterBase::ModernLogger::Debug("Evicting distant chunk at ({}, {})", it->sectorX, it->sectorY);
                UserInterface::Core::EventBus::GetInstance().Publish(ChunkUnloadRequestEvent{*it});
                it = m_activeChunks.erase(it);
            } else {
                ++it;
            }
        }
    }

    void ClearAllChunks() override {
        for (const auto& coord : m_activeChunks) {
            UserInterface::Core::EventBus::GetInstance().Publish(ChunkUnloadRequestEvent{coord});
        }
        m_activeChunks.clear();
        EterBase::ModernLogger::Info("All map chunks cleared.");
    }

private:
    std::unordered_set<ChunkCoordinate, ChunkCoordinateHash, ChunkCoordinateEqual> m_activeChunks;

    [[nodiscard]] std::expected<ChunkCoordinate, EterBase::PacketError> CalculateSector(float x, float y) const {
        if (std::isnan(x) || std::isnan(y) || std::isinf(x) || std::isinf(y)) {
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }
        
        int32_t sectorX = static_cast<int32_t>(std::floor(x / SECTOR_SIZE));
        int32_t sectorY = static_cast<int32_t>(std::floor(y / SECTOR_SIZE));
        return ChunkCoordinate{sectorX, sectorY};
    }
};

} // namespace GameLib::Terrain
