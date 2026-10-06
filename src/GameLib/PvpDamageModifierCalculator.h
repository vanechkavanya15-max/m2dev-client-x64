#pragma once

#include <cstdint>
#include <optional>
#include "../EterBase/StrongTypes.h"
#include "../EterBase/Result.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"
#include "RaceManagerLite.h"

/**
 * @file PvpDamageModifierCalculator.h
 * @brief Modern C++23 calculator for PvP damage modifiers based on player race classes.
 * 
 * Replaces the old procedural logic with a clean, decoupled class that utilizes
 * EterBase strong types, std::expected for robust error handling, and emits
 * events via the Core::EventBus instead of direct UI calls.
 */

namespace GameLib {

/**
 * @brief Represents the data payload emitted when a PvP damage modifier is successfully calculated.
 */
struct PvpDamageCalculatedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId attackerId;
    EterBase::EntityId victimId;
    float baseDamage;
    float finalDamage;
    float modifierMultiplier;

    /**
     * @brief Constructs a new Pvp Damage Calculated Event object.
     * 
     * @param attackerId EntityId of the attacking player.
     * @param victimId EntityId of the victim player.
     * @param baseDamage The initial damage amount before modifiers.
     * @param finalDamage The calculated damage amount after modifiers.
     * @param modifierMultiplier The exact multiplier applied to the base damage.
     */
    PvpDamageCalculatedEvent(EterBase::EntityId attackerId, EterBase::EntityId victimId, float baseDamage, float finalDamage, float modifierMultiplier)
        : attackerId(attackerId), victimId(victimId), baseDamage(baseDamage), finalDamage(finalDamage), modifierMultiplier(modifierMultiplier) {}
};

/**
 * @brief Calculator for applying PvP-specific damage modifiers.
 */
class PvpDamageModifierCalculator {
public:
    /**
     * @brief Gets the singleton instance of the calculator.
     * 
     * @return PvpDamageModifierCalculator& 
     */
    static PvpDamageModifierCalculator& GetInstance() {
        static PvpDamageModifierCalculator instance;
        return instance;
    }

    /**
     * @brief Calculates the final damage applied from one player to another based on race PvP modifiers.
     * 
     * @param attackerId The EntityId of the attacking actor.
     * @param victimId The EntityId of the victim actor.
     * @param attackerRaceVnum The VNUM indicating the race of the attacker.
     * @param victimRaceVnum The VNUM indicating the race of the victim.
     * @param baseDamage The calculated base damage before PvP-specific adjustments.
     * @return EterBase::Result<float, EterBase::CombatError> The calculated final damage, or an error code on failure.
     */
    EterBase::Result<float, EterBase::CombatError> CalculatePvpDamage(
        EterBase::EntityId attackerId, 
        EterBase::EntityId victimId, 
        uint32_t attackerRaceVnum, 
        uint32_t victimRaceVnum, 
        float baseDamage) const 
    {
        if (baseDamage < 0.0f) {
            EterBase::ModernLogger::Warn("CalculatePvpDamage called with negative base damage for Attacker ID: {}", attackerId.value());
            return EterBase::MakeError(EterBase::CombatError::InvalidAction);
        }

        // Only process damage if both are players
        if (!RaceManagerLite::GetInstance().IsPlayer(attackerRaceVnum) || 
            !RaceManagerLite::GetInstance().IsPlayer(victimRaceVnum)) {
            
            EterBase::ModernLogger::Debug("Skipping PvP damage calculation; one or both entities are not players.");
            // If they are not players, we just return base damage (not an error, just no PvP modifier).
            return baseDamage;
        }

        float modifier = GetPvpModifier(attackerRaceVnum, victimRaceVnum).value_or(1.0f);
        float finalDamage = baseDamage * modifier;

        EterBase::ModernLogger::Debug(
            "PvP Damage Calculated. Attacker: {}, Victim: {}, Base: {}, Final: {}, Modifier: {}", 
            attackerId.value(), victimId.value(), baseDamage, finalDamage, modifier);

        // Publish event to decouple from GUI
        UserInterface::Core::EventBus::GetInstance().Publish(
            PvpDamageCalculatedEvent(attackerId, victimId, baseDamage, finalDamage, modifier)
        );

        return finalDamage;
    }

private:
    PvpDamageModifierCalculator() = default;
    ~PvpDamageModifierCalculator() = default;
    PvpDamageModifierCalculator(const PvpDamageModifierCalculator&) = delete;
    PvpDamageModifierCalculator& operator=(const PvpDamageModifierCalculator&) = delete;

    /**
     * @brief Retrieves the specific damage modifier based on the attacker and victim's race VNUMs.
     * 
     * @param attackerRaceVnum Attacker's race VNUM.
     * @param victimRaceVnum Victim's race VNUM.
     * @return std::optional<float> The modifier multiplier if applicable, or std::nullopt.
     */
    std::optional<float> GetPvpModifier(uint32_t attackerRaceVnum, uint32_t victimRaceVnum) const {
        // Example base IDs:
        // Warrior M = 0, Ninja W = 1, Sura M = 2, Shaman W = 3
        // Warrior W = 4, Ninja M = 5, Sura W = 6, Shaman M = 7
        // Wolfman M = 8

        // Normalize race to base classes for calculation (0=Warrior, 1=Ninja, 2=Sura, 3=Shaman, 4=Wolfman)
        auto normalizeRace = [](uint32_t vnum) -> std::optional<uint8_t> {
            switch(vnum) {
                case 0: case 4: return 0; // Warrior
                case 1: case 5: return 1; // Ninja
                case 2: case 6: return 2; // Sura
                case 3: case 7: return 3; // Shaman
                case 8:         return 4; // Wolfman
                default:        return std::nullopt;
            }
        };

        auto optAttackerClass = normalizeRace(attackerRaceVnum);
        auto optVictimClass = normalizeRace(victimRaceVnum);

        return optAttackerClass.and_then([&](uint8_t attackerClass) {
            return optVictimClass.transform([&](uint8_t victimClass) -> float {
                return CalculateClassModifier(attackerClass, victimClass);
            });
        });
    }

    /**
     * @brief Calculates the specific multiplier between two classes.
     * 
     * @param attackerClass The normalized class ID of the attacker.
     * @param victimClass The normalized class ID of the victim.
     * @return float The damage multiplier.
     */
    float CalculateClassModifier(uint8_t attackerClass, uint8_t victimClass) const {
        // Hardcoded generic class balacing coefficients for standard 2026 PvP.
        // In a full implementation, these could be loaded from a configuration or EventBus request.
        
        // Sura vs Sura damage reduction
        if (attackerClass == 2 && victimClass == 2) {
            return 0.9f;
        }

        // Shaman receives slightly more damage from Warrior
        if (attackerClass == 0 && victimClass == 3) {
            return 1.05f;
        }

        // Ninja deals slightly more damage to Sura
        if (attackerClass == 1 && victimClass == 2) {
            return 1.10f;
        }

        // Wolfman deals more damage to Shaman, receives more from Ninja
        if (attackerClass == 4 && victimClass == 3) {
            return 1.15f;
        }
        if (attackerClass == 1 && victimClass == 4) {
            return 1.10f;
        }

        // Default multiplier
        return 1.0f;
    }
};

} // namespace GameLib
