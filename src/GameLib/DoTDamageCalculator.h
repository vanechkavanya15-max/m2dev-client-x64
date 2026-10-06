#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>
#include <format>

#include "../EterBase/StrongTypes.h"
#include "../EterBase/ModernLogger.h"
#include "../EterBase/PacketResult.h"
#include "Core/EventBus.h"

namespace GameLib::CombatMath {

    /**
     * @brief Represents the type of Damage over Time (DoT).
     */
    enum class DoTType : uint8_t {
        Poison = 0,
        Bleeding = 1
    };

    /**
     * @brief Combat statistics representing DoT properties.
     */
    struct DoTStats {
        uint32_t baseDamage; ///< Base damage applied per tick.
        uint32_t duration;   ///< Duration in seconds or ticks.
    };

    /**
     * @brief Event emitted when DoT damage is calculated and applied.
     */
    struct DoTDamageEvent : public Core::IEvent {
        EterBase::EntityId attackerId;
        EterBase::EntityId victimId;
        uint32_t damage;
        DoTType type;

        /**
         * @brief Constructs a DoTDamageEvent.
         * @param attacker The ID of the attacker causing the DoT.
         * @param victim The ID of the victim receiving the DoT.
         * @param damage The calculated damage value.
         * @param type The type of DoT applied.
         */
        DoTDamageEvent(EterBase::EntityId attacker, EterBase::EntityId victim, uint32_t damage, DoTType type)
            : attackerId(attacker), victimId(victim), damage(damage), type(type) {}
    };

    /**
     * @brief A stateless calculator for Damage over Time effects.
     */
    class DoTDamageCalculator {
    public:
        /**
         * @brief Calculates the total DoT damage based on the given stats and emits a damage event.
         * 
         * @param type The type of Damage over Time (Poison, Bleeding).
         * @param attacker The entity ID of the attacker.
         * @param victim The entity ID of the victim.
         * @param stats Optional containing the DoT statistics (base damage, duration).
         * @return EterBase::PacketResult<uint32_t> The calculated damage if successful, otherwise an error message.
         */
        static EterBase::PacketResult<uint32_t> CalculateDamage(
            DoTType type,
            EterBase::EntityId attacker,
            EterBase::EntityId victim,
            std::optional<DoTStats> stats) 
        {
            return stats.and_then([&](const DoTStats& s) -> std::optional<EterBase::PacketResult<uint32_t>> {
                uint32_t calculatedDamage = s.baseDamage * s.duration;
                if (calculatedDamage == 0) {
                    return std::unexpected("Calculated damage is zero");
                }

                // Decouple from GUI, emit event to the Core EventBus.
                Core::EventBus::Instance().Publish(
                    DoTDamageEvent{attacker, victim, calculatedDamage, type}
                );

                EterBase::ModernLogger::Info(
                    "Calculated {} damage from Attacker {} to Victim {}", 
                    calculatedDamage, attacker.value(), victim.value()
                );
                return calculatedDamage;
            }).value_or(std::unexpected("Missing DoTStats"));
        }
    };

} // namespace GameLib::CombatMath
