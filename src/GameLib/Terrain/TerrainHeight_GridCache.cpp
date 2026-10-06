#include "../StdAfx.h"
#include "ITerrainHeightCache.h"

#include "EterBase/ModernLogger.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "UserInterface/Core/EventBus.h"

#include <unordered_map>
#include <shared_mutex>
#include <cmath>
#include <cstdint>
#include <memory>

namespace GameLib::Terrain
{
    /**
     * @brief Zdarzenie rozglaszane, gdy cache zostal wyczyszczony.
     */
    struct TerrainHeightCacheClearedEvent : public UserInterface::Core::IEvent {
        TerrainHeightCacheClearedEvent() = default;
    };

    /**
     * @brief Szybka tablica kafelkow probkowanych w pamieci RAM do blyskawicznego odczytu wysokosci Z.
     * Zgodna z SRP, zapewnia bezpieczny dostep wielowatkowy uzywajac shared_mutex.
     */
    class TerrainHeight_GridCache final : public ITerrainHeightCache
    {
    public:
        TerrainHeight_GridCache()
        {
            EterBase::ModernLogger::Info("TerrainHeight_GridCache initialized.");
        }

        ~TerrainHeight_GridCache() override
        {
            EterBase::ModernLogger::Info("TerrainHeight_GridCache destroyed.");
        }

        /**
         * @brief Pobiera z pamieci podrecznej wysokosc Z dla danych koordynat.
         * @param x Koordynata X.
         * @param y Koordynata Y.
         * @return Wysokosc terenu (Z).
         */
        [[nodiscard]] float SampleHeight(float x, float y) const override
        {
            return GetCell(x, y).z;
        }

        /**
         * @brief Pobiera z pamieci podrecznej wysokosc wody dla danych koordynat.
         * @param x Koordynata X.
         * @param y Koordynata Y.
         * @return Wysokosc wody.
         */
        [[nodiscard]] float SampleWaterHeight(float x, float y) const override
        {
            return GetCell(x, y).waterZ;
        }

        /**
         * @brief Weryfikuje, czy dany punkt pozwala na swobodne chodzenie bazujac na kacie nachylenia.
         * @param x Koordynata X.
         * @param y Koordynata Y.
         * @param maxSlopeAngle Maksymalny dopuszczalny kat nachylenia.
         * @return True jesli teren jest pochyly w dopuszczalnych granicach, w przeciwnym razie False.
         */
        [[nodiscard]] bool IsWalkableSlope(float x, float y, float maxSlopeAngle) const override
        {
            return GetCell(x, y).slopeAngle <= maxSlopeAngle;
        }

        /**
         * @brief Pobiera hurtowo wysokosci Z dla duzej ilosci punktow (np. AI Navigation).
         * @param x Tablica wspolrzednych X.
         * @param y Tablica wspolrzednych Y.
         * @param outZ Tablica wyjsciowa do zapisania wynikow Z.
         * @param count Liczba elementow do przetworzenia.
         */
        void BatchSampleHeight(const float* x, const float* y, float* outZ, size_t count) const override
        {
            if (!x || !y || !outZ) {
                EterBase::ModernLogger::Error("TerrainHeight_GridCache::BatchSampleHeight: Nullptr passed");
                return;
            }

            std::shared_lock lock(m_mutex);
            for (size_t i = 0; i < count; ++i) {
                uint64_t key = PackCoordinates(x[i], y[i]);
                auto it = m_cache.find(key);
                outZ[i] = (it != m_cache.end()) ? it->second.z : 0.0f;
            }
        }

        /**
         * @brief Usuwa calkowicie pamiec podreczna (np. po wyladowaniu strefy mapy).
         */
        void Clear() override
        {
            {
                std::unique_lock lock(m_mutex);
                m_cache.clear();
            }
            EterBase::ModernLogger::Info("TerrainHeight_GridCache::Clear: Cache cleared.");
            UserInterface::Core::EventBus::GetInstance().Publish(TerrainHeightCacheClearedEvent{});
        }

        /**
         * @brief Wypelnia pamiec podreczna z systemow zewnetrznych. Uzywa std::expected do obslugi ewentualnych bledow domeny.
         * @param x Koordynata X.
         * @param y Koordynata Y.
         * @param z Srednia wysokosc Z terenu.
         * @param waterZ Srednia wysokosc Z wody.
         * @param slopeAngle Kat nachylenia.
         * @return VoidResult.
         */
        std::expected<void, std::string_view> UpdateCell(float x, float y, float z, float waterZ, float slopeAngle)
        {
            if (std::isnan(x) || std::isnan(y)) {
                return std::unexpected("Invalid coordinates NaN");
            }
            
            uint64_t key = PackCoordinates(x, y);
            std::unique_lock lock(m_mutex);
            m_cache[key] = {z, waterZ, slopeAngle};
            return {};
        }

    private:
        struct CellInfo {
            float z = 0.0f;
            float waterZ = 0.0f;
            float slopeAngle = 0.0f;
        };

        [[nodiscard]] CellInfo GetCell(float x, float y) const
        {
            uint64_t key = PackCoordinates(x, y);
            std::shared_lock lock(m_mutex);
            auto it = m_cache.find(key);
            if (it != m_cache.end()) {
                return it->second;
            }
            return CellInfo{};
        }

        // Przykladowy optymalny rozmiar kafelka do probkowania w Metin2
        static constexpr float GRID_CELL_SIZE = 100.0f;

        [[nodiscard]] static uint64_t PackCoordinates(float x, float y) noexcept
        {
            int32_t ix = static_cast<int32_t>(std::floor(x / GRID_CELL_SIZE));
            int32_t iy = static_cast<int32_t>(std::floor(y / GRID_CELL_SIZE));
            return (static_cast<uint64_t>(static_cast<uint32_t>(ix)) << 32) | static_cast<uint32_t>(iy);
        }

        mutable std::shared_mutex m_mutex;
        mutable std::unordered_map<uint64_t, CellInfo> m_cache;
    };

    /**
     * @brief Factory function do tworzenia instancji ITerrainHeightCache.
     * Umozliwia wstrzykiwanie zaleznosci i eksport podsystemu.
     * @return Nowa instancja cache'u terenu.
     */
    std::unique_ptr<ITerrainHeightCache> CreateTerrainHeightGridCache()
    {
        return std::make_unique<TerrainHeight_GridCache>();
    }
}
