#pragma once

#include <cstdint>
#include <EterBase/Result.h>

/**
 * @file DropProbabilityMath.h
 * @brief Implementacja modulu kalkulacji szansy na drop przedmiotu.
 * 
 * Modul opiera sie na silnikach bezstanowych, korzystajac z C++23.
 * Do celow diagnostycznych uzywany jest system bledow.
 */

namespace Client::Formulas {

    /// @brief Enum okreslajacy rodzaj bledu w przypadku nieudanej kalkulacji
    enum class DropProbabilityError : uint8_t {
        None = 0,
        BaseProbabilityNegative,
        InvalidEventBonus
    };

    /// @brief Zwraca reprezentacje tekstowa bledu (przydatne do logowania).
    [[nodiscard]] constexpr std::string_view ToString(DropProbabilityError err) noexcept {
        switch (err) {
            case DropProbabilityError::None: return "None";
            case DropProbabilityError::BaseProbabilityNegative: return "BaseProbabilityNegative";
            case DropProbabilityError::InvalidEventBonus: return "InvalidEventBonus";
        }
        return "UnknownDropProbabilityError";
    }

    /// @brief Struktura opisujaca aktywne modyfikatory szansy
    struct DropModifiers {
        bool hasPremiumThiefGlove = false; // Premium Thief Glove (z ItemShop, +100%)
        bool hasInGameThiefGlove = false;  // In-Game Thief Glove (np. z Lasu, +50%)
        uint32_t eventDropBonusPercent = 0; // Dodatkowy bonus procentowy z eventu
    };

    /**
     * @brief Oblicza koncowy mnoznik dropu na podstawie posiadanych rekwic i eventow.
     * @param modifiers Struktura z flagami modyfikatorow
     * @return Result z koncowym mnoznikiem bazowym
     */
    [[nodiscard]] constexpr EterBase::Result<double, DropProbabilityError> CalculateDropMultiplier(
        const DropModifiers& modifiers) noexcept {

        double multiplier = 1.0; // Bazowa szansa = 1x (100%)

        if (modifiers.hasPremiumThiefGlove) {
            multiplier += 1.0;
        }

        if (modifiers.hasInGameThiefGlove) {
            multiplier += 0.5;
        }

        if (modifiers.eventDropBonusPercent > 1000) { 
            // Ograniczenie np. 1000% dla sanity checka.
            return std::unexpected(DropProbabilityError::InvalidEventBonus);
        }
        
        multiplier += static_cast<double>(modifiers.eventDropBonusPercent) / 100.0;

        return multiplier;
    }

    /**
     * @brief Oblicza calkowita szanse na wydropienie danego przedmiotu z uwzglednieniem bazy i mnoznikow.
     * 
     * @param baseProbability Poczatkowa bazowa szansa na drop [0.0, 1.0].
     * @param modifiers Modyfikatory (rekawice i bonusy).
     * @return EterBase::Result z finalna szansa na drop, zaleznie od modyfikatorow.
     */
    [[nodiscard]] constexpr EterBase::Result<double, DropProbabilityError> CalculateFinalDropProbability(
        double baseProbability, 
        const DropModifiers& modifiers) noexcept {

        if (baseProbability < 0.0) {
            return std::unexpected(DropProbabilityError::BaseProbabilityNegative);
        }

        const auto multiplierResult = CalculateDropMultiplier(modifiers);
        if (!multiplierResult.has_value()) {
            return std::unexpected(multiplierResult.error());
        }

        double finalProbability = baseProbability * multiplierResult.value();
        
        return finalProbability;
    }

} // namespace Client::Formulas
