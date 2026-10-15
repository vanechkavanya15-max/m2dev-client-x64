#pragma once

#include <cstdint>
#include <optional>
#include <format>
#include <algorithm>

#include "../EterBase/Result.h"
#include "../EterBase/StrongTypes.h"
#include "../EterBase/LogModern.h"

// Forward declare random_range to avoid pulling in stdlib.h's random() conflict on Linux via <format>
extern long random_range(long from, long to);

#include "../UserInterface/Core/EventBus.h"

namespace Core {
    class EventBus {
    public:
        static UserInterface::Core::EventBus& Instance() {
            return UserInterface::Core::EventBus::GetInstance();
        }
    };
}

namespace CombatMath {

    /**
     * @brief Event emitted when a critical strike is successfully executed.
     */
    struct CriticalStrikeEvent : public UserInterface::Core::IEvent {
        EterBase::EntityId attackerId;
        EterBase::EntityId defenderId;
        uint32_t finalDamage;

        /**
         * @brief Constructs a new Critical Strike Event.
         * @param attackerId The ID of the attacker.
         * @param defenderId The ID of the defender.
         * @param finalDamage The calculated damage.
         */
        CriticalStrikeEvent(EterBase::EntityId attackerId, EterBase::EntityId defenderId, uint32_t finalDamage)
            : attackerId(attackerId), defenderId(defenderId), finalDamage(finalDamage) {}
    };

    /**
     * @brief Event emitted when a piercing strike is successfully executed.
     */
    struct PiercingStrikeEvent : public UserInterface::Core::IEvent {
        EterBase::EntityId attackerId;
        EterBase::EntityId defenderId;
        uint32_t finalDamage;

        /**
         * @brief Constructs a new Piercing Strike Event.
         * @param attackerId The ID of the attacker.
         * @param defenderId The ID of the defender.
         * @param finalDamage The calculated damage.
         */
        PiercingStrikeEvent(EterBase::EntityId attackerId, EterBase::EntityId defenderId, uint32_t finalDamage)
            : attackerId(attackerId), defenderId(defenderId), finalDamage(finalDamage) {}
    };

    /**
     * @brief Struct representing the inputs for a critical/pierce strike calculation.
     */
    struct StrikeAttributes {
        EterBase::EntityId attackerId;      ///< Attacker Entity ID
        EterBase::EntityId defenderId;      ///< Defender Entity ID
        uint32_t baseDamage;                ///< The base damage before critical/pierce calculation
        uint32_t targetDefense;             ///< The defender's defense value
        uint8_t criticalChance;             ///< Percentage chance for a critical hit (0-100)
        uint8_t pierceChance;               ///< Percentage chance for a piercing hit (0-100)
    };

    /**
     * @brief Struct representing the result of the damage calculation.
     */
    struct StrikeResult {
        uint32_t damage;    ///< The final calculated damage
        bool isCritical;    ///< True if the attack was critical
        bool isPiercing;    ///< True if the attack was piercing
    };

    /**
     * @brief Calculator for handling critical and piercing strike multipliers and chances.
     */
    class CriticalPierceCalculator {
    public:
        /**
         * @brief Calculates the final damage considering critical and piercing mechanics.
         * 
         * @param attributes The strike attributes containing chances and base values.
         * @return EterBase::Result<StrikeResult, EterBase::CombatError> The calculated damage result or a combat error.
         */
        [[nodiscard]] static EterBase::Result<StrikeResult, EterBase::CombatError> CalculateStrike(const StrikeAttributes& attributes) noexcept {
            if (!attributes.attackerId || !attributes.defenderId) {
                EterBase::ModernLogger::Error("CriticalPierceCalculator: Invalid attacker or defender ID.");
                return EterBase::MakeError(EterBase::CombatError::InvalidAction);
            }

            uint32_t damage = attributes.baseDamage;
            bool isCritical = false;
            bool isPiercing = false;

            // Calculate Piercing (ignores defense and adds it to damage)
            std::optional<uint32_t> pierceDamage = std::nullopt;
            if (attributes.pierceChance > 0 && random_range(1, 100) <= attributes.pierceChance) {
                pierceDamage = attributes.targetDefense;
            }

            damage += pierceDamage.value_or(0);
            isPiercing = pierceDamage.has_value();

            if (isPiercing) {
                EterBase::ModernLogger::Debug("CriticalPierceCalculator: Piercing strike occurred (Attacker: {}, Defender: {})", attributes.attackerId.get(), attributes.defenderId.get());
                Core::EventBus::Instance().Publish(PiercingStrikeEvent(attributes.attackerId, attributes.defenderId, damage));
            }

            // Calculate Critical (doubles damage)
            std::optional<uint32_t> criticalMultiplier = std::nullopt;
            if (attributes.criticalChance > 0 && random_range(1, 100) <= attributes.criticalChance) {
                criticalMultiplier = 2;
            }

            damage = criticalMultiplier.transform([damage](uint32_t mult) { return damage * mult; }).value_or(damage);
            isCritical = criticalMultiplier.has_value();

            if (isCritical) {
                EterBase::ModernLogger::Debug("CriticalPierceCalculator: Critical strike occurred (Attacker: {}, Defender: {})", attributes.attackerId.get(), attributes.defenderId.get());
                Core::EventBus::Instance().Publish(CriticalStrikeEvent(attributes.attackerId, attributes.defenderId, damage));
            }

            return StrikeResult{ damage, isCritical, isPiercing };
        }
    };

} // namespace CombatMath
