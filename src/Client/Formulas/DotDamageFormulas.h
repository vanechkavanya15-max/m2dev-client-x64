#pragma once

#include <cstdint>
#include <expected>

namespace Client::Formulas {

    enum class DotError : uint8_t {
        InvalidMaxHp,
        InvalidCurrentHp,
        InvalidPercentage
    };

    template <typename T>
    using Result = std::expected<T, DotError>;

    struct DotDamageResult {
        int64_t damageAmount;
        int64_t newHp;
    };

    /**
     * Oblicza obrazenia z trucizny z uwzglednieniem ochrony przed zabiciem potwora.
     * Trucizna to staly procent od maksymalnego HP.
     * W przypadku potworow, jesli obrazenia mialyby zabic, HP zatrzymuje sie na 1.
     */
    [[nodiscard]] constexpr Result<DotDamageResult> CalculatePoisonDamage(
        int64_t currentHp, 
        int64_t maxHp, 
        int64_t poisonPercentage, 
        bool isMonster) noexcept 
    {
        if (maxHp <= 0) {
            return std::unexpected(DotError::InvalidMaxHp);
        }
        if (currentHp <= 0) {
            return std::unexpected(DotError::InvalidCurrentHp);
        }
        if (poisonPercentage < 0) {
            return std::unexpected(DotError::InvalidPercentage);
        }
        
        int64_t effectiveHp = currentHp;
        if (effectiveHp > maxHp) {
            effectiveHp = maxHp;
        }

        int64_t damageAmount = (maxHp * poisonPercentage) / 100;
        if (damageAmount == 0 && poisonPercentage > 0) {
            damageAmount = 1; // Minimalne obrazenia dla malego max HP
        }

        int64_t newHp = effectiveHp - damageAmount;

        // Ochrona potworow przed smiercia od trucizny
        if (isMonster && newHp <= 0) {
            newHp = 1;
            damageAmount = effectiveHp - 1;
        }

        // Gracz moze umrzec od trucizny
        if (newHp < 0) {
            newHp = 0;
            damageAmount = effectiveHp;
        }

        return DotDamageResult{damageAmount, newHp};
    }

    /**
     * Oblicza obrazenia z krwawienia z uwzglednieniem ochrony przed zabiciem potwora.
     * Podobnie jak trucizna, krwawienie to procent od max HP i nie zabija potworow.
     * Sluzy jako zrodlo DoT uzywane w nowoczesnych systemach.
     */
    [[nodiscard]] constexpr Result<DotDamageResult> CalculateBleedingDamage(
        int64_t currentHp, 
        int64_t maxHp, 
        int64_t bleedingPercentage, 
        bool isMonster) noexcept 
    {
        if (maxHp <= 0) {
            return std::unexpected(DotError::InvalidMaxHp);
        }
        if (currentHp <= 0) {
            return std::unexpected(DotError::InvalidCurrentHp);
        }
        if (bleedingPercentage < 0) {
            return std::unexpected(DotError::InvalidPercentage);
        }

        int64_t effectiveHp = currentHp;
        if (effectiveHp > maxHp) {
            effectiveHp = maxHp;
        }

        int64_t damageAmount = (maxHp * bleedingPercentage) / 100;
        if (damageAmount == 0 && bleedingPercentage > 0) {
            damageAmount = 1; 
        }

        int64_t newHp = effectiveHp - damageAmount;

        // Ochrona potworow przed smiercia od krwawienia
        if (isMonster && newHp <= 0) {
            newHp = 1;
            damageAmount = effectiveHp - 1;
        }

        // Gracz moze umrzec od krwawienia
        if (newHp < 0) {
            newHp = 0;
            damageAmount = effectiveHp;
        }

        return DotDamageResult{damageAmount, newHp};
    }

} // namespace Client::Formulas
