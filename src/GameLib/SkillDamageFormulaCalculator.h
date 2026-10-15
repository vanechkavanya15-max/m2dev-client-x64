#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <format>
#include <algorithm>
#include "../EterBase/StrongTypes.h"

/**
 * @file SkillDamageFormulaCalculator.h
 * @brief Modern C++23 calculator for skill damage formulas (Warrior, Ninja, Sura, Shaman).
 * 
 * Provides pure mathematical formulas for calculating skill damages.
 * Uses C++23 std::expected for robust error handling and EterBase::StrongType for type safety.
 * This class is completely decoupled from the GUI and intended for event-driven backend use.
 */

namespace CombatMath {

    /**
     * @brief Error codes that can occur during skill damage calculation.
     */
    enum class CalculationError {
        None,
        InvalidSkillId,
        AttackerStatsMissing,
        DefenderStatsMissing,
        LevelTooLow,
        CalculationOverflow
    };

    /**
     * @brief Contains the base stats of an attacker necessary for skill calculation.
     */
    struct AttackerStats {
        EterBase::EntityId id;
        EterBase::PlayerLevel level;
        uint32_t strength;
        uint32_t dexterity;
        uint32_t intelligence;
        uint32_t vitality;
        uint32_t baseAttackPower;
        uint32_t magicAttackPower;
    };

    /**
     * @brief Contains the defensive stats of a defender.
     */
    struct DefenderStats {
        EterBase::EntityId id;
        EterBase::PlayerLevel level;
        uint32_t defense;
        uint32_t magicDefense;
        uint8_t skillResistancePercent; // 0-100
    };

    /**
     * @brief Pure static class for calculating skill damage per character class.
     */
    class SkillDamageFormulaCalculator {
    public:
        /**
         * @brief Calculates the skill damage for a Warrior character.
         * 
         * @param skill The ID of the skill being used.
         * @param attacker The stats of the attacking warrior.
         * @param defender The stats of the defending entity.
         * @param skillLevel The level/grade of the skill (e.g., 1-40).
         * @return std::expected<uint32_t, CalculationError> The final calculated damage, or an error code.
         */
        static constexpr std::expected<uint32_t, CalculationError> CalculateWarriorSkillDamage(
            EterBase::SkillId skill,
            const std::optional<AttackerStats>& attacker,
            const std::optional<DefenderStats>& defender,
            uint8_t skillLevel
        ) noexcept;

        /**
         * @brief Calculates the skill damage for a Ninja character.
         * 
         * @param skill The ID of the skill being used.
         * @param attacker The stats of the attacking ninja.
         * @param defender The stats of the defending entity.
         * @param skillLevel The level/grade of the skill.
         * @return std::expected<uint32_t, CalculationError> The final calculated damage, or an error code.
         */
        static constexpr std::expected<uint32_t, CalculationError> CalculateNinjaSkillDamage(
            EterBase::SkillId skill,
            const std::optional<AttackerStats>& attacker,
            const std::optional<DefenderStats>& defender,
            uint8_t skillLevel
        ) noexcept;

        /**
         * @brief Calculates the skill damage for a Sura character.
         * 
         * @param skill The ID of the skill being used.
         * @param attacker The stats of the attacking sura.
         * @param defender The stats of the defending entity.
         * @param skillLevel The level/grade of the skill.
         * @return std::expected<uint32_t, CalculationError> The final calculated damage, or an error code.
         */
        static constexpr std::expected<uint32_t, CalculationError> CalculateSuraSkillDamage(
            EterBase::SkillId skill,
            const std::optional<AttackerStats>& attacker,
            const std::optional<DefenderStats>& defender,
            uint8_t skillLevel
        ) noexcept;

        /**
         * @brief Calculates the skill damage for a Shaman character.
         * 
         * @param skill The ID of the skill being used.
         * @param attacker The stats of the attacking shaman.
         * @param defender The stats of the defending entity.
         * @param skillLevel The level/grade of the skill.
         * @return std::expected<uint32_t, CalculationError> The final calculated damage, or an error code.
         */
        static constexpr std::expected<uint32_t, CalculationError> CalculateShamanSkillDamage(
            EterBase::SkillId skill,
            const std::optional<AttackerStats>& attacker,
            const std::optional<DefenderStats>& defender,
            uint8_t skillLevel
        ) noexcept;
    };

} // namespace CombatMath

namespace CombatMath {

    // ------------------------------------------------------------------------
    // Implementation of inline / constexpr methods
    // ------------------------------------------------------------------------

    constexpr std::expected<uint32_t, CalculationError> SkillDamageFormulaCalculator::CalculateWarriorSkillDamage(
        EterBase::SkillId skill,
        const std::optional<AttackerStats>& attacker,
        const std::optional<DefenderStats>& defender,
        uint8_t skillLevel
    ) noexcept {
        if (!attacker.has_value()) return std::unexpected(CalculationError::AttackerStatsMissing);
        if (!defender.has_value()) return std::unexpected(CalculationError::DefenderStatsMissing);
        if (skill.get() == 0) return std::unexpected(CalculationError::InvalidSkillId);
        
        const auto& att = attacker.value();
        const auto& def = defender.value();

        // Warrior formula heavily relies on Strength and Base Attack Power.
        // Base damage: Attack * 1.5 + (STR * 2.0)
        float baseDamage = static_cast<float>(att.baseAttackPower) * 1.5f + static_cast<float>(att.strength) * 2.0f;
        
        // Skill multiplier based on level (simplified)
        float skillMultiplier = 1.0f + (static_cast<float>(skillLevel) * 0.05f);
        
        float rawDamage = baseDamage * skillMultiplier;
        
        // Mitigation
        float mitigatedDamage = rawDamage - static_cast<float>(def.defense);
        if (mitigatedDamage < 1.0f) mitigatedDamage = 1.0f;

        // Resistances
        float resistMultiplier = 1.0f - (std::min<uint8_t>(def.skillResistancePercent, 100) / 100.0f);
        float finalDamage = mitigatedDamage * resistMultiplier;

        return static_cast<uint32_t>(std::max(1.0f, finalDamage));
    }

    constexpr std::expected<uint32_t, CalculationError> SkillDamageFormulaCalculator::CalculateNinjaSkillDamage(
        EterBase::SkillId skill,
        const std::optional<AttackerStats>& attacker,
        const std::optional<DefenderStats>& defender,
        uint8_t skillLevel
    ) noexcept {
        if (!attacker.has_value()) return std::unexpected(CalculationError::AttackerStatsMissing);
        if (!defender.has_value()) return std::unexpected(CalculationError::DefenderStatsMissing);
        if (skill.get() == 0) return std::unexpected(CalculationError::InvalidSkillId);

        const auto& att = attacker.value();
        const auto& def = defender.value();

        // Ninja formula heavily relies on Dexterity.
        // Base damage: Attack * 1.0 + (DEX * 2.5) + (STR * 0.5)
        float baseDamage = static_cast<float>(att.baseAttackPower) * 1.0f + static_cast<float>(att.dexterity) * 2.5f + static_cast<float>(att.strength) * 0.5f;
        
        float skillMultiplier = 1.0f + (static_cast<float>(skillLevel) * 0.06f);
        
        float rawDamage = baseDamage * skillMultiplier;
        
        float mitigatedDamage = rawDamage - static_cast<float>(def.defense);
        if (mitigatedDamage < 1.0f) mitigatedDamage = 1.0f;

        float resistMultiplier = 1.0f - (std::min<uint8_t>(def.skillResistancePercent, 100) / 100.0f);
        float finalDamage = mitigatedDamage * resistMultiplier;

        return static_cast<uint32_t>(std::max(1.0f, finalDamage));
    }

    constexpr std::expected<uint32_t, CalculationError> SkillDamageFormulaCalculator::CalculateSuraSkillDamage(
        EterBase::SkillId skill,
        const std::optional<AttackerStats>& attacker,
        const std::optional<DefenderStats>& defender,
        uint8_t skillLevel
    ) noexcept {
        if (!attacker.has_value()) return std::unexpected(CalculationError::AttackerStatsMissing);
        if (!defender.has_value()) return std::unexpected(CalculationError::DefenderStatsMissing);
        if (skill.get() == 0) return std::unexpected(CalculationError::InvalidSkillId);

        const auto& att = attacker.value();
        const auto& def = defender.value();

        // Sura formula relies on Intelligence and Magic Attack Power.
        // Base damage: Magic Attack * 1.2 + Attack * 0.8 + (INT * 2.2)
        float baseDamage = static_cast<float>(att.magicAttackPower) * 1.2f + static_cast<float>(att.baseAttackPower) * 0.8f + static_cast<float>(att.intelligence) * 2.2f;
        
        float skillMultiplier = 1.0f + (static_cast<float>(skillLevel) * 0.055f);
        
        float rawDamage = baseDamage * skillMultiplier;
        
        // Suras deal magic damage, mitigate with magic defense
        float mitigatedDamage = rawDamage - static_cast<float>(def.magicDefense);
        if (mitigatedDamage < 1.0f) mitigatedDamage = 1.0f;

        float resistMultiplier = 1.0f - (std::min<uint8_t>(def.skillResistancePercent, 100) / 100.0f);
        float finalDamage = mitigatedDamage * resistMultiplier;

        return static_cast<uint32_t>(std::max(1.0f, finalDamage));
    }

    constexpr std::expected<uint32_t, CalculationError> SkillDamageFormulaCalculator::CalculateShamanSkillDamage(
        EterBase::SkillId skill,
        const std::optional<AttackerStats>& attacker,
        const std::optional<DefenderStats>& defender,
        uint8_t skillLevel
    ) noexcept {
        if (!attacker.has_value()) return std::unexpected(CalculationError::AttackerStatsMissing);
        if (!defender.has_value()) return std::unexpected(CalculationError::DefenderStatsMissing);
        if (skill.get() == 0) return std::unexpected(CalculationError::InvalidSkillId);

        const auto& att = attacker.value();
        const auto& def = defender.value();

        // Shaman formula relies heavily on Intelligence.
        // Base damage: Magic Attack * 1.5 + (INT * 2.5)
        float baseDamage = static_cast<float>(att.magicAttackPower) * 1.5f + static_cast<float>(att.intelligence) * 2.5f;
        
        float skillMultiplier = 1.0f + (static_cast<float>(skillLevel) * 0.05f);
        
        float rawDamage = baseDamage * skillMultiplier;
        
        // Shamans deal magic damage
        float mitigatedDamage = rawDamage - static_cast<float>(def.magicDefense);
        if (mitigatedDamage < 1.0f) mitigatedDamage = 1.0f;

        float resistMultiplier = 1.0f - (std::min<uint8_t>(def.skillResistancePercent, 100) / 100.0f);
        float finalDamage = mitigatedDamage * resistMultiplier;

        return static_cast<uint32_t>(std::max(1.0f, finalDamage));
    }

} // namespace CombatMath
