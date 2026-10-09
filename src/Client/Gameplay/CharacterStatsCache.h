#pragma once

#include <cstdint>
#include <array>
#include <shared_mutex>
#include <atomic>
#include <unordered_map>
#include <expected>

namespace Client::Gameplay {

enum class CharacterStatsError : uint8_t {
    None = 0,
    IndexOutOfBounds,
    Overflow
};

template <typename T>
using StatsResult = std::expected<T, CharacterStatsError>;

/**
 * @brief Podreczny cache statystyk postaci zapewniajacy szybki dostep bez przeliczania co klatke.
 */
class CharacterStatsCache {
public:
    static constexpr size_t MAX_STAT_TYPES = 256;

    CharacterStatsCache() noexcept = default;
    ~CharacterStatsCache() noexcept = default;

    // Kopiowanie i przenoszenie zabronione
    CharacterStatsCache(const CharacterStatsCache&) = delete;
    CharacterStatsCache& operator=(const CharacterStatsCache&) = delete;

    /**
     * @brief Aktualizuje zsumowane bonusy na podstawie dostarczonej mapy.
     */
    void UpdateBonuses(const std::unordered_map<uint8_t, int32_t>& newBonuses) noexcept;

    /**
     * @brief Pobiera wartosc bonusu dla danego typu. Zwraca blad jesli indeks jest nieprawidlowy.
     */
    StatsResult<int32_t> GetBonusValue(uint8_t bonusType) const noexcept;

    /**
     * @brief Ustawia bazowa wartosc statystyki. Zwraca blad jesli indeks jest nieprawidlowy.
     */
    StatsResult<void> SetBaseStat(uint8_t statType, int32_t value) noexcept;

    /**
     * @brief Pobiera bazowa wartosc statystyki. Zwraca blad jesli indeks jest nieprawidlowy.
     */
    StatsResult<int32_t> GetBaseStat(uint8_t statType) const noexcept;

    /**
     * @brief Pobiera calkowita wartosc statystyki (baza + bonus). Zwraca blad w przypadku przepelnienia lub blednego indeksu.
     */
    StatsResult<int32_t> GetTotalStat(uint8_t statType) const noexcept;

    /**
     * @brief Oznacza cache jako nieaktualny.
     */
    void MarkDirty() noexcept;

    /**
     * @brief Sprawdza, czy cache wymaga przeliczenia.
     */
    bool IsDirty() const noexcept;

    /**
     * @brief Czysci wszystkie zbuforowane dane.
     */
    void Clear() noexcept;

private:
    mutable std::shared_mutex m_mutex;
    std::array<int32_t, MAX_STAT_TYPES> m_baseStats{};
    std::array<int32_t, MAX_STAT_TYPES> m_bonusStats{};
    std::atomic<bool> m_isDirty{true};
};

} // namespace Client::Gameplay
