#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <format>
#include <string>
#include <cmath>

#include "../EterBase/StrongTypes.h"
#include "../EterBase/LogModern.h"
#include "../EterBase/Result.h"
#include "../UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Event emitted when an entity's movement velocity is calculated.
 */
struct VelocityCalculatedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    float displacementPerMs;

    /**
     * @brief Constructs a VelocityCalculatedEvent.
     * @param id The entity ID.
     * @param displacement The calculated displacement per millisecond.
     */
    VelocityCalculatedEvent(EterBase::EntityId id, float displacement)
        : entityId(id), displacementPerMs(displacement) {}
};

/**
 * @brief Calculator for movement displacement per millisecond.
 * 
 * Handles the calculation of an entity's speed converting from base and modifier
 * into a per-millisecond displacement float value.
 */
class MovementVelocityCalculator {
public:
    /**
     * @brief Calculates the displacement per millisecond for an entity.
     * 
     * @param id Strong type EntityId representing the actor.
     * @param baseSpeed The base movement speed of the entity.
     * @param speedModifier Optional modifier to the base speed (e.g., buffs, debuffs).
     * @return std::expected<float, EterBase::PacketError> The displacement per ms, or an error.
     */
    [[nodiscard]] static std::expected<float, EterBase::PacketError> CalculateDisplacementPerMs(
        EterBase::EntityId id,
        int32_t baseSpeed,
        std::optional<int32_t> speedModifier = std::nullopt) noexcept
    {
        // Avoid deep nested if pyramids by using value_or
        const int32_t modifier = speedModifier.value_or(0);
        int32_t totalSpeed = baseSpeed + modifier;

        // Ensure speed is non-negative
        if (totalSpeed < 0) {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Warning, "EntityId {} calculated negative speed {}, clamping to 0", id.value(), totalSpeed);
            totalSpeed = 0;
        }

        // According to the specification, ratio is divided by 1000 to get per ms speed
        const float displacement = static_cast<float>(totalSpeed) / 1000.0f;

        // Log the successful calculation securely via std::format backend in ModernLogger
        EterBase::ModernLogger::Log(EterBase::LogLevel::Info, "Calculated displacement for EntityId {}: {} per ms", id.value(), displacement);

        // Emit the event via the EventBus
        UserInterface::Core::EventBus::GetInstance().Publish(VelocityCalculatedEvent{id, displacement});

        return displacement;
    }
};

} // namespace GameLib
