#pragma once

#include <cstdint>
#include <expected>
#include <algorithm>
#include <cmath>

#include "../EterBase/Result.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Structure holding calculated penalties for experience and item drops.
 */
struct PenaltyFactors {
    float expMultiplier;
    float dropMultiplier;
};

/**
 * @brief Event emitted when a level difference penalty is calculated and applied.
 */
struct LevelPenaltyAppliedEvent : public UserInterface::Core::IEvent {
    uint32_t attackerLevel;
    uint32_t victimLevel;
    PenaltyFactors factors;

    /**
     * @brief Constructs the event.
     * @param attackerLevel Level of the attacking entity.
     * @param victimLevel Level of the victim entity.
     * @param factors The calculated penalty factors.
     */
    LevelPenaltyAppliedEvent(uint32_t attackerLevel, uint32_t victimLevel, PenaltyFactors factors)
        : attackerLevel(attackerLevel), victimLevel(victimLevel), factors(factors) {}
};

/**
 * @brief Calculator for computing combat penalties based on level differences.
 */
class LevelDifferencePenaltyCalculator {
public:
    /**
     * @brief Calculates the penalty for experience and drops based on level difference.
     * 
     * @param attackerLevel Level of the attacker.
     * @param victimLevel Level of the victim.
     * @return std::expected<PenaltyFactors, EterBase::EntityError> The calculated penalty factors or an error if levels are invalid.
     */
    [[nodiscard]] static std::expected<PenaltyFactors, EterBase::EntityError> CalculatePenalty(
        uint32_t attackerLevel, uint32_t victimLevel) 
    {
        if (attackerLevel == 0 || victimLevel == 0) {
            EterBase::ModernLogger::Warn("Invalid level provided to LevelDifferencePenaltyCalculator");
            return std::unexpected(EterBase::EntityError::InvalidType);
        }

        PenaltyFactors factors{1.0f, 1.0f};

        // Difference is typically calculated as Attacker Level - Victim Level
        // If attacker is much higher level than the victim, penalties apply.
        int32_t levelDifference = static_cast<int32_t>(attackerLevel) - static_cast<int32_t>(victimLevel);

        if (levelDifference > 14) {
            factors.expMultiplier = 0.0f;
            factors.dropMultiplier = 0.0f;
        } else if (levelDifference > 10) {
            // Linear drop-off for differences between 11 and 14
            // 11 -> 0.8, 12 -> 0.6, 13 -> 0.4, 14 -> 0.2
            float penalty = 1.0f - (static_cast<float>(levelDifference - 10) * 0.2f);
            factors.expMultiplier = penalty;
            factors.dropMultiplier = penalty;
        } else if (levelDifference < -10) {
            // If the victim is much higher level, perhaps slight penalty or boost
            // Typically in Metin2, no penalty for fighting higher levels, 
            // but we can ensure it stays at 1.0f
            factors.expMultiplier = 1.0f;
            factors.dropMultiplier = 1.0f;
        }

        // Publish event to decouple from GUI
        UserInterface::Core::EventBus::GetInstance().Publish(
            LevelPenaltyAppliedEvent(attackerLevel, victimLevel, factors)
        );

        return factors;
    }
};

} // namespace GameLib
