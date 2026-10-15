#pragma once

#include <expected>
#include <optional>
#include <cstdint>
#include <format>
#include <span>

#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Enum representing the different elemental types for combat.
 */
enum class ElementalType : uint8_t {
    Fire,
    Ice,
    Lightning,
    Wind,
    Earth,
    Dark
};

/**
 * @brief Enum representing errors that can occur during elemental damage calculation.
 */
enum class ElementalCalculateError : uint8_t {
    InvalidInputParameters,
    MissingDamageOrDefenseValue,
    NegativeDamageOrDefense,
    CalculationOverflow
};

/**
 * @brief Event emitted when elemental damage has been successfully calculated.
 * 
 * Decoupled from the GUI via the EventBus architecture.
 */
struct ElementalDamageCalculatedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId attackerId;
    EterBase::EntityId victimId;
    ElementalType elementType;
    int32_t originalDamage;
    int32_t finalDamage;

    /**
     * @brief Constructs the event.
     * @param attacker The ID of the attacking entity.
     * @param victim The ID of the defending entity.
     * @param element The elemental type used.
     * @param origDmg The raw damage before defense reduction.
     * @param finalDmg The damage calculated after defense reduction.
     */
    ElementalDamageCalculatedEvent(EterBase::EntityId attacker, EterBase::EntityId victim, 
                                   ElementalType element, int32_t origDmg, int32_t finalDmg)
        : attackerId(attacker), victimId(victim), elementType(element), 
          originalDamage(origDmg), finalDamage(finalDmg) {}
};

/**
 * @brief Calculator for elemental damage and defense.
 *
 * Provides purely mathematical operations and validates logic using modern C++ standards,
 * adhering to the Zero-Conflict Rule and GUI detachment.
 */
class ElementalDefenseCalculator {
public:
    /**
     * @brief Calculates final damage based on element type and defense.
     *
     * Utilizes std::expected for robust error handling instead of out parameters,
     * std::optional with monadic ops for parameter verification, and StrongTypes.
     *
     * @param attacker The attacking entity ID.
     * @param victim The defending entity ID.
     * @param element The elemental type of the attack.
     * @param baseDamage The base elemental damage, if any.
     * @param elementalDefense The victim's elemental defense value, if any.
     * @return The final calculated damage or an ElementalCalculateError if invalid.
     */
    static std::expected<int32_t, ElementalCalculateError> CalculateDamage(
        EterBase::EntityId attacker,
        EterBase::EntityId victim,
        ElementalType element,
        std::optional<int32_t> baseDamage,
        std::optional<int32_t> elementalDefense) 
    {
        // Require valid entities
        if (!attacker || !victim) {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "Elemental calculation failed: invalid EntityId(s) (attacker: {}, victim: {})", attacker.value(), victim.value());
            return std::unexpected(ElementalCalculateError::InvalidInputParameters);
        }
        
        // Monadic operation to verify and transform damage values
        auto finalResult = baseDamage
            .and_then([](int32_t dmg) -> std::optional<int32_t> {
                if (dmg < 0) return std::nullopt; // Protect against negative damage
                return dmg;
            })
            .and_then([&](int32_t validDmg) -> std::optional<int32_t> {
                return elementalDefense.transform([&](int32_t def) {
                    if (def < 0) def = 0; // Negative defense clamped to 0
                    
                    // Simple logic: defense reduces damage by percentage max 90% or flat reduction
                    // Assuming flat reduction for this context:
                    int32_t finalDmg = std::max<int32_t>(0, validDmg - def);
                    return finalDmg;
                });
            });

        if (!finalResult.has_value()) {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Warning, "Elemental calculation failed: invalid or missing damage/defense for attacker {} vs victim {}", attacker.value(), victim.value());
            return std::unexpected(ElementalCalculateError::MissingDamageOrDefenseValue);
        }

        int32_t finalDmg = finalResult.value();

        EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "Elemental damage calculated. Attacker: {}, Victim: {}, Element: {}, Base: {}, Final: {}",
            attacker.value(), victim.value(), static_cast<uint32_t>(element), baseDamage.value(), finalDmg);

        // Publish event to decouple logic from UI / networking 
        ElementalDamageCalculatedEvent evt(attacker, victim, element, baseDamage.value(), finalDmg);
        UserInterface::Core::EventBus::GetInstance().Publish(evt);

        return finalDmg;
    }
};

} // namespace GameLib
