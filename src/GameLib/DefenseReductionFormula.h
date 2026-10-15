#pragma once

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"
#include <optional>
#include <expected>
#include <algorithm>

/**
 * @file DefenseReductionFormula.h
 * @brief Formulas for calculating physical damage reduction based on target defense.
 * 
 * Implements modern C++23 standards, strong types, and EventBus for GUI decoupling.
 */

namespace BattleCalculator {

/**
 * @brief Event published when physical defense reduction is applied.
 */
struct DefenseReductionAppliedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId attackerId;
    EterBase::EntityId defenderId;
    float originalDamage;
    float reducedDamage;

    /**
     * @brief Constructs the event.
     * @param attacker The attacker's entity ID.
     * @param defender The defender's entity ID.
     * @param orig The original raw damage.
     * @param reduced The final reduced damage.
     */
    DefenseReductionAppliedEvent(EterBase::EntityId attacker, EterBase::EntityId defender, float orig, float reduced)
        : attackerId(attacker), defenderId(defender), originalDamage(orig), reducedDamage(reduced) {}
};

/**
 * @brief Defense stats specifically used for reduction calculation.
 */
struct DefenseReductionStats {
    uint32_t baseDefense;      ///< Base defense value of the entity.
    uint32_t level;            ///< Level of the entity.
    uint8_t physicalResist;    ///< Physical resistance percentage (0-100).
};

/**
 * @class DefenseReductionFormula
 * @brief Provides logic for calculating damage reduction from defense stats.
 */
class DefenseReductionFormula {
public:
    /**
     * @brief Calculates reduced physical damage based on defense and resistances.
     * 
     * @param attackerId Entity ID of the attacker.
     * @param defenderId Entity ID of the defender.
     * @param rawDamage The initial physical damage amount before reduction.
     * @param defenderStats The defense statistics of the defender. If std::nullopt, no reduction is applied.
     * @return EterBase::Result<float, EterBase::CombatError> The final reduced damage or a CombatError.
     */
    static EterBase::Result<float, EterBase::CombatError> CalculatePhysicalReduction(
        EterBase::EntityId attackerId,
        EterBase::EntityId defenderId,
        float rawDamage,
        const std::optional<DefenseReductionStats>& defenderStats)
    {
        if (!attackerId) {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "CalculatePhysicalReduction: Invalid attackerId");
            return std::unexpected(EterBase::CombatError::InvalidAction);
        }

        if (!defenderId) {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "CalculatePhysicalReduction: Invalid defenderId");
            return std::unexpected(EterBase::CombatError::TargetNotFound);
        }

        if (rawDamage <= 0.0f) {
            return 0.0f;
        }

        float finalDamage = defenderStats.transform([&](const DefenseReductionStats& stats) {
            float mitigated = rawDamage - static_cast<float>(stats.baseDefense);
            if (mitigated < 1.0f) {
                mitigated = 1.0f; // Minimum damage before resistance is 1
            }
            
            float resistMultiplier = std::max(0.0f, 1.0f - (std::min<uint8_t>(stats.physicalResist, 100) / 100.0f));
            return std::max(1.0f, mitigated * resistMultiplier);
        }).value_or(rawDamage);

        EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, 
            "Physical damage reduction applied. Attacker: {}, Defender: {}, Raw: {}, Final: {}",
            attackerId.get(), defenderId.get(), rawDamage, finalDamage);

        // Publish event for decoupling
        Core::EventBus::Instance().Publish(
            DefenseReductionAppliedEvent(attackerId, defenderId, rawDamage, finalDamage)
        );

        return finalDamage;
    }
};

} // namespace BattleCalculator
