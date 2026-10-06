#pragma once

#include <cstdint>
#include <optional>
#include <expected>
#include <format>
#include "../EterBase/Result.h"
#include "../EterBase/StrongTypes.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"

namespace GameLib::CombatMath {

/**
 * @brief Represents the type of mount the character is riding.
 */
enum class MountType : uint8_t {
    None = 0,
    Horse = 1,
    SpecialMount = 2
};

/**
 * @brief Represents combat modifiers provided by a mount.
 */
struct MountModifiers {
    uint32_t attackBonus;    ///< Flat bonus to attack.
    uint32_t defenseBonus;   ///< Flat bonus to defense.
    float attackMultiplier;  ///< Multiplier for attack (e.g., 1.1f for 10% bonus).
    float defenseMultiplier; ///< Multiplier for defense.
};

/**
 * @brief Event published when a mount modifier is calculated or changes.
 */
struct MountModifierCalculatedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    MountModifiers modifiers;

    /**
     * @brief Constructs the event with calculated modifiers.
     * @param entityId The entity ID receiving the modifiers.
     * @param modifiers The calculated modifiers.
     */
    MountModifierCalculatedEvent(EterBase::EntityId entityId, MountModifiers modifiers)
        : entityId(entityId), modifiers(modifiers) {}
};

/**
 * @brief Calculator for attack and defense modifiers when fighting from a mount.
 * 
 * Uses C++23 features like std::expected, std::optional, and strong types.
 * Adheres to Zero-Conflict rule by remaining standalone and event-driven.
 */
class MountCombatModifierCalculator {
public:
    /**
     * @brief Calculates the combat modifiers for an entity based on its mount and level.
     * 
     * @param entityId The unique identifier of the entity.
     * @param mountType The type of mount currently being ridden.
     * @param mountLevel The level of the mount (optional, defaults to 1).
     * @param playerLevel The level of the player.
     * @return std::expected<MountModifiers, EterBase::CombatError> The calculated modifiers or an error.
     */
    static std::expected<MountModifiers, EterBase::CombatError> CalculateModifiers(
        EterBase::EntityId entityId,
        MountType mountType,
        std::optional<EterBase::PlayerLevel> mountLevel,
        EterBase::PlayerLevel playerLevel) noexcept 
    {
        if (mountType == MountType::None) {
            EterBase::ModernLogger::Debug("Entity {} has no mount, returning default modifiers.", entityId.value());
            return MountModifiers{0, 0, 1.0f, 1.0f};
        }

        // Use monadic operations on optional
        uint8_t effectiveMountLevel = mountLevel.transform([](EterBase::PlayerLevel level) {
            return level.value();
        }).value_or(1);

        uint8_t currentLevel = playerLevel.value();

        if (currentLevel < 25 && mountType == MountType::Horse) {
            EterBase::ModernLogger::Warn("Entity {} is level {} and cannot benefit from horse combat (min level 25).", entityId.value(), currentLevel);
            return std::unexpected(EterBase::CombatError::InvalidAction);
        }

        MountModifiers modifiers{0, 0, 1.0f, 1.0f};

        switch (mountType) {
            case MountType::Horse:
                modifiers.attackBonus = effectiveMountLevel * 2;
                modifiers.defenseBonus = effectiveMountLevel * 1;
                modifiers.attackMultiplier = 1.0f + (effectiveMountLevel * 0.01f);
                break;
            case MountType::SpecialMount:
                modifiers.attackBonus = 50 + (effectiveMountLevel * 3);
                modifiers.defenseBonus = 30 + (effectiveMountLevel * 2);
                modifiers.attackMultiplier = 1.1f + (effectiveMountLevel * 0.015f);
                modifiers.defenseMultiplier = 1.05f + (effectiveMountLevel * 0.01f);
                break;
            default:
                return std::unexpected(EterBase::CombatError::InvalidAction);
        }

        EterBase::ModernLogger::Info("Calculated mount modifiers for entity {}: Attack Bonus: {}, Defense Bonus: {}", 
            entityId.value(), modifiers.attackBonus, modifiers.defenseBonus);

        // Publish event to decouple from UI
        UserInterface::Core::EventBus::GetInstance().Publish(
            MountModifierCalculatedEvent(entityId, modifiers)
        );

        return modifiers;
    }
};

} // namespace GameLib::CombatMath
