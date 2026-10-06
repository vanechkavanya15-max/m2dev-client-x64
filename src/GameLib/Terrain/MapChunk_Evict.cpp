#include "../StdAfx.h"
#include "IMapChunkStreamingService.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../../UserInterface/Core/EventBus.h"

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <expected>

namespace {
    // Definiowanie struktury zdarzenia do wyslania na szyne zdarzen (EventBus)
    struct ChunkEvictedEvent : public UserInterface::Core::IEvent {
        GameLib::Terrain::ChunkCoordinate coord;

        explicit ChunkEvictedEvent(GameLib::Terrain::ChunkCoordinate coord) : coord(coord) {}
    };

    // Hashing struktury ChunkCoordinate dla uzycia w std::unordered_map
    struct ChunkHash {
        std::size_t operator()(const GameLib::Terrain::ChunkCoordinate& coord) const noexcept {
            std::size_t h1 = std::hash<int32_t>{}(coord.sectorX);
            std::size_t h2 = std::hash<int32_t>{}(coord.sectorY);
            return h1 ^ (h2 << 1);
        }
    };


    // Stan globalny w tym module do sledzenia czasu ostatniego dostepu wedlug polityki LRU
    std::unordered_map<GameLib::Terrain::ChunkCoordinate, std::chrono::steady_clock::time_point, ChunkHash> g_chunkAccessTimes;

    // Domyslny rozmiar sektora (magiczna liczba do przeliczen, zwykle 25600 lub inna domyslna wartosc)
    constexpr float SECTOR_SIZE = 25600.0f;
}

namespace GameLib::Terrain {

    /**
     * @brief Zwalnia pamiec oddalonych kafelkow mapy (chunks) zgodnie z polityka LRU.
     * 
     * Implementacja modulowa, wykorzystujaca C++23, silne typy i event bus.
     */
    std::expected<void, std::string_view> EvictDistantChunks(float playerX, float playerY, float maxRadius) {
        if (maxRadius <= 0.0f) {
            EterBase::ModernLogger::Warning("EvictDistantChunks: invalid maxRadius provided.");
            return std::unexpected("Invalid radius");
        }

        if (g_chunkAccessTimes.empty()) {
            return {}; // Nie ma czego wyrzucac
        }

        float maxRadiusSq = maxRadius * maxRadius;
        std::vector<ChunkCoordinate> toEvict;

        for (const auto& [coord, accessTime] : g_chunkAccessTimes) {
            float chunkWorldX = coord.sectorX * SECTOR_SIZE;
            float chunkWorldY = coord.sectorY * SECTOR_SIZE;

            float dx = chunkWorldX - playerX;
            float dy = chunkWorldY - playerY;
            float distanceSq = (dx * dx) + (dy * dy);

            if (distanceSq > maxRadiusSq) {
                toEvict.push_back(coord);
            }
        }

        if (toEvict.empty()) {
            return {};
        }

        // Sortowanie wytypowanych kafelkow mapy pod wzgledem LRU, aby najstarsze usunac na pewno jako pierwsze
        std::sort(toEvict.begin(), toEvict.end(), [](const ChunkCoordinate& a, const ChunkCoordinate& b) {
            return g_chunkAccessTimes[a] < g_chunkAccessTimes[b];
        });

        // Ewikcja
        for (const auto& coord : toEvict) {
            g_chunkAccessTimes.erase(coord);
            
            EterBase::ModernLogger::Info("Evicted chunk (SectorX: {}, SectorY: {}) due to distance limit.", coord.sectorX, coord.sectorY);
            
            UserInterface::Core::EventBus::GetInstance().Publish(ChunkEvictedEvent{coord});
        }

        return {};
    }

} // namespace GameLib::Terrain
