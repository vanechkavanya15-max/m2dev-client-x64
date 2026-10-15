#pragma once

#include <cstdint>
#include <algorithm>

/**
 * @file BattleCalculator.h
 * @brief Pure C++20 damage calculation formula calculator for CombatMath domain.
 *
 * It provides mathematical operations for combat damage calculations,
 * without UI decoupling and following modern C++ standards.
 */

namespace BattleCalculator
{
    /**
     * @brief Represents the type of damage applied.
     */
    enum class DamageType : uint8_t
    {
        Physical = 0,
        Magic = 1,
        Sword = 2,
        TwoHanded = 3,
        Dagger = 4,
        Bell = 5,
        Fan = 6,
        Bow = 7,
        Claw = 8
    };

    /**
     * @brief Combat statistics representing an actor's attack properties.
     */
    struct AttackStats
    {
        uint32_t baseAttack;   ///< Base attack value.
        uint32_t level;        ///< Attacker's level.
        float skillMultiplier; ///< Skill damage multiplier.
    };

    /**
     * @brief Combat statistics representing a defender's resistance and defense properties.
     */
    struct DefenseStats
    {
        uint32_t baseDefense;      ///< Base defense value.
        uint32_t level;            ///< Defender's level.
        uint8_t physicalResist;    ///< Physical resistance percentage (0-100).
        uint8_t magicResist;       ///< Magic resistance percentage (0-100).
        uint8_t weaponResist;      ///< Weapon specific resistance percentage (0-100).
    };

    /**
     * @brief Calculates the final damage applied to a defender.
     *
     * @param attackerStats The attack attributes of the damage dealer.
     * @param defenderStats The defensive attributes of the damage receiver.
     * @param type The type of damage being dealt.
     * @return uint32_t The final calculated damage. Minimum is 1 if attack is successful but fully mitigated.
     */
    constexpr uint32_t CalculateDamage(const AttackStats& attackerStats, const DefenseStats& defenderStats, DamageType type) noexcept
    {
        // Calculate raw damage (base attack * skill multiplier)
        float rawDamage = static_cast<float>(attackerStats.baseAttack) * std::max(0.0f, attackerStats.skillMultiplier);

        // Calculate defense mitigation
        float defenseMitigation = static_cast<float>(defenderStats.baseDefense);

        // Apply level difference factor (simple implementation for base formula)
        float levelFactor = 1.0f;
        if (attackerStats.level > defenderStats.level) {
            levelFactor += (attackerStats.level - defenderStats.level) * 0.01f;
        } else if (defenderStats.level > attackerStats.level) {
            levelFactor -= (defenderStats.level - attackerStats.level) * 0.01f;
        }
        
        levelFactor = std::clamp(levelFactor, 0.5f, 1.5f);

        // Apply base damage calculation
        float mitigatedDamage = (rawDamage - defenseMitigation) * levelFactor;
        if (mitigatedDamage <= 0.0f) {
            mitigatedDamage = 1.0f; // Minimum 1 damage if hit
        }

        // Apply resistances based on damage type
        float resistanceMultiplier = 1.0f;

        if (type == DamageType::Magic) {
            uint8_t effectiveResist = std::min<uint8_t>(defenderStats.magicResist, 100);
            resistanceMultiplier -= effectiveResist / 100.0f;
        } else {
            uint8_t effectiveResist = std::min<uint8_t>(defenderStats.physicalResist, 100);
            resistanceMultiplier -= effectiveResist / 100.0f;
            
            // Apply weapon specific resistance if applicable
            if (type != DamageType::Physical) {
                uint8_t effectiveWeaponResist = std::min<uint8_t>(defenderStats.weaponResist, 100);
                resistanceMultiplier *= (1.0f - (effectiveWeaponResist / 100.0f));
            }
        }
        
        resistanceMultiplier = std::max(0.0f, resistanceMultiplier);

        float finalDamage = mitigatedDamage * resistanceMultiplier;

        return static_cast<uint32_t>(std::max(1.0f, finalDamage));
    }
}
