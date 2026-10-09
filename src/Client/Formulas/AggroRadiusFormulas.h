#pragma once

#include <expected>
#include <cstdint>
#include <algorithm>

namespace Client::Formulas
{
    enum class AggroRadiusError
    {
        InvalidBaseRadius,
        InvalidMonsterLevel,
        InvalidTargetLevel,
    };

    /**
     * @brief Oblicza zmodyfikowany promien agresji potwora w zaleznosci od roznicy poziomow.
     * 
     * Formula:
     * - Domyslnie agresja jest rowna baseRadius.
     * - Jesli poziom celu jest wiekszy od poziomu potwora, promien agresji zalezy od roznicy poziomow.
     *   Za kazdy poziom powyzej poziomu potwora, promien agresji moze malec.
     * - W tym przypadku, jezeli roznica poziomow jest wyzsza lub rowna 10, zasieg agresji maleje o 50%.
     * - Jesli roznica poziomow to np. 5-9, moze malec proporcjonalnie.
     * - Mozemy uzyc prostej kalkulacji: za kazdy poziom wiecej celu niz potwora, zasieg maleje o 2%, maksymalnie o 50%.
     * 
     * Parametry:
     * @param baseRadius Bazowy zasieg w jednostkach.
     * @param monsterLevel Poziom potwora.
     * @param targetLevel Poziom celu (gracza).
     * 
     * @return Zmodyfikowany zasieg jako float lub kod bledu z enumeratora AggroRadiusError.
     */
    [[nodiscard]] constexpr std::expected<float, AggroRadiusError> CalculateAggroRadius(
        float baseRadius,
        uint32_t monsterLevel,
        uint32_t targetLevel) noexcept
    {
        if (baseRadius <= 0.0f)
        {
            return std::unexpected(AggroRadiusError::InvalidBaseRadius);
        }

        if (monsterLevel == 0)
        {
            return std::unexpected(AggroRadiusError::InvalidMonsterLevel);
        }

        if (targetLevel == 0)
        {
            return std::unexpected(AggroRadiusError::InvalidTargetLevel);
        }

        if (targetLevel <= monsterLevel)
        {
            return baseRadius;
        }

        const uint32_t levelDifference = targetLevel - monsterLevel;
        
        // Zmniejszenie zasiegu o 2% za kazdy poziom roznicy
        const float penaltyPercentage = static_cast<float>(levelDifference) * 0.02f;
        
        // Maksymalna redukcja to 50%
        const float clampedPenalty = std::min(penaltyPercentage, 0.5f);

        const float modifiedRadius = baseRadius * (1.0f - clampedPenalty);

        // Zabezpieczenie przez ujemnymi wartosciami (tu nie powinno wystapic, ale w razie rozszerzen)
        return std::max(modifiedRadius, 0.0f);
    }

} // namespace Client::Formulas
