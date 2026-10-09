#pragma once

#include <cstdint>
#include <span>
#include <expected>
#include <algorithm>
#include <limits>
#include <string_view>
#include <format>

namespace Client::Formulas {

/**
 * @brief Enumy bledow matematyki punktow doswiadczenia.
 */
enum class ExpMathError : uint8_t {
    None = 0,
    InvalidLevel,
    MaxLevelReached,
    TableOutOfBounds,
    CalculationOverflow
};

template <typename T>
using ExpResult = std::expected<T, ExpMathError>;

[[nodiscard]] constexpr std::string_view to_string(ExpMathError error) noexcept {
    switch (error) {
        case ExpMathError::None: return "ExpMathError::None";
        case ExpMathError::InvalidLevel: return "ExpMathError::InvalidLevel - Podany poziom jest nieprawidlowy";
        case ExpMathError::MaxLevelReached: return "ExpMathError::MaxLevelReached - Osiagnieto maksymalny poziom";
        case ExpMathError::TableOutOfBounds: return "ExpMathError::TableOutOfBounds - Brak danych o doswiadczeniu dla podanego poziomu";
        case ExpMathError::CalculationOverflow: return "ExpMathError::CalculationOverflow - Przepelnienie podczas obliczania doswiadczenia";
        default: return "ExpMathError::Unknown";
    }
}

/**
 * @brief Bezstanowa klasa matematyczna obliczajaca zaleznosci doswiadczenia od poziomow.
 */
class ExpLevelTableMath {
public:
    /**
     * @brief Zwraca ilosc punktow doswiadczenia wymagana do awansu z podanego poziomu.
     * 
     * @param currentLevel Aktualny poziom postaci.
     * @param expTable Tablica progow doswiadczenia, gdzie indeks to poziom.
     * @return ExpResult<uint64_t> Wymagane doswiadczenie do awansu.
     */
    [[nodiscard]] static constexpr ExpResult<uint64_t> GetNextLevelExp(uint32_t currentLevel, std::span<const uint64_t> expTable) noexcept {
        if (expTable.empty()) {
            return std::unexpected(ExpMathError::TableOutOfBounds);
        }
        
        if (currentLevel >= expTable.size()) {
            return std::unexpected(ExpMathError::MaxLevelReached);
        }
        
        return expTable[currentLevel];
    }

    /**
     * @brief Oblicza mnoznik doswiadczenia bazujac na roznicy poziomow gracza i potwora.
     * 
     * @param playerLevel Poziom gracza.
     * @param mobLevel Poziom potwora.
     * @return double Mnoznik doswiadczenia, np. 0.0 - 1.5.
     */
    [[nodiscard]] static constexpr double CalculateLevelDifferenceMultiplier(uint32_t playerLevel, uint32_t mobLevel) noexcept {
        if (playerLevel > mobLevel) {
            const uint32_t diff = playerLevel - mobLevel;
            if (diff >= 15) {
                return 0.0;
            }
            if (diff >= 6) {
                return 1.0 - static_cast<double>(diff - 5) * 0.1;
            }
            return 1.0;
        } 
        
        const uint32_t diff = mobLevel - playerLevel;
        if (diff >= 10) {
            return 1.5;
        }
        if (diff >= 5) {
            return 1.2;
        }
        
        return 1.0;
    }

    /**
     * @brief Oblicza ostateczna wartosc zdobytego doswiadczenia unikajac przepelnien arytmetycznych.
     * Uzywa arytmetyki stalopozycyjnej by ograniczyc utrate precyzji podczas obliczen float.
     * 
     * @param baseExp Bazowe doswiadczenie do zdobycia.
     * @param playerLevel Poziom gracza.
     * @param mobLevel Poziom potwora.
     * @return ExpResult<uint64_t> Skalkulowane i znormalizowane doswiadczenie.
     */
    [[nodiscard]] static constexpr ExpResult<uint64_t> CalculateGainedExp(uint64_t baseExp, uint32_t playerLevel, uint32_t mobLevel) noexcept {
        const double multiplierDouble = CalculateLevelDifferenceMultiplier(playerLevel, mobLevel);
        if (multiplierDouble <= 0.0) {
            return 0;
        }

        // Przejscie na promile aby uniknac operacji na ulamkach zmiennoprzecinkowych dla glownej puli exp.
        // Uzywamy bezposredniego rzutowania, poniewaz std::round nie jest w pelni wspierane w constexpr we wszystkich kompilatorach.
        const uint64_t multiplierPermille = static_cast<uint64_t>(multiplierDouble * 1000.0);
        
        const uint64_t q = baseExp / 1000;
        const uint64_t r = baseExp % 1000;
        
        // Sprawdzamy czy mnozenie w czesci calkowitej (q) nie spowoduje przepelnienia typu uint64_t
        if (q > std::numeric_limits<uint64_t>::max() / multiplierPermille) {
            return std::unexpected(ExpMathError::CalculationOverflow);
        }
        
        const uint64_t part1 = q * multiplierPermille;
        const uint64_t part2 = (r * multiplierPermille) / 1000;
        
        // Sprawdzamy czy ostateczne dodawanie z reszta nie przekroczy zakresu
        if (std::numeric_limits<uint64_t>::max() - part1 < part2) {
            return std::unexpected(ExpMathError::CalculationOverflow);
        }
        
        return part1 + part2;
    }
};

} // namespace Client::Formulas

// Wsparcie dla std::format
template <>
struct std::formatter<Client::Formulas::ExpMathError> : std::formatter<std::string_view> {
    auto format(Client::Formulas::ExpMathError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Formulas::to_string(err), ctx);
    }
};
