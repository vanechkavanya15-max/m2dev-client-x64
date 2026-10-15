#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <format>
#include <algorithm>

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

/**
 * @file CastTimeSpeedCalculator.h
 * @brief Pure C++23 calculator for determining actual skill cast time based on cast speed.
 *
 * It provides operations for calculating cast time with optional speed overrides,
 * utilizing strong types, monads and decoupling from UI via EventBus.
 */

namespace CombatMath
{
    /**
     * @brief Event published when a skill's cast time is successfully calculated.
     */
    struct SkillCastTimeCalculatedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId casterId;
        EterBase::SkillId skillId;
        float actualCastTime;

        /**
         * @brief Constructs the event.
         * @param caster The entity casting the skill.
         * @param skill The skill being cast.
         * @param time The calculated cast time in seconds.
         */
        SkillCastTimeCalculatedEvent(EterBase::EntityId caster, EterBase::SkillId skill, float time)
            : casterId(caster), skillId(skill), actualCastTime(time) {}
    };

    /**
     * @brief Calculator for casting time operations.
     */
    class CastTimeSpeedCalculator
    {
    public:
        /**
         * @brief Calculates the actual cast time for a skill.
         * 
         * Calculation formula: Actual Cast Time = Base Cast Time / (Cast Speed / 100.0f).
         * 
         * @param casterId The ID of the entity casting the skill.
         * @param skillId The ID of the skill.
         * @param baseCastTime The base cast time of the skill in seconds.
         * @param castSpeed The default cast speed percentage (e.g., 100.0f for 100%).
         * @param castSpeedOverride An optional override for the cast speed percentage.
         * @return EterBase::Result<float, EterBase::EntityError> The calculated cast time on success, or an error.
         */
        [[nodiscard]] static EterBase::Result<float, EterBase::EntityError> CalculateCastTime(
            EterBase::EntityId casterId,
            EterBase::SkillId skillId,
            float baseCastTime,
            float castSpeed,
            std::optional<float> castSpeedOverride = std::nullopt) noexcept
        {
            if (!casterId)
            {
                EterBase::ModernLogger::Error("Failed to calculate cast time: Invalid caster ID");
                return std::unexpected(EterBase::EntityError::NotFound);
            }

            if (baseCastTime < 0.0f)
            {
                EterBase::ModernLogger::Error("Failed to calculate cast time for caster {}: Negative base cast time {}", casterId.get(), baseCastTime);
                return std::unexpected(EterBase::EntityError::InvalidType);
            }

            // Determine effective cast speed using monadic operations
            float effectiveCastSpeed = castSpeedOverride
                .transform([](float overrideSpeed) { return std::max(1.0f, overrideSpeed); })
                .value_or(std::max(1.0f, castSpeed));

            float actualCastTime = baseCastTime / (effectiveCastSpeed / 100.0f);

            EterBase::ModernLogger::Info(
                "Calculated cast time for caster {} skill {}: Base = {}, Speed = {}, Actual = {}",
                casterId.get(), skillId.get(), baseCastTime, effectiveCastSpeed, actualCastTime
            );

            // Publish event
            SkillCastTimeCalculatedEvent event{casterId, skillId, actualCastTime};
            UserInterface::Core::EventBus::GetInstance().Publish(event);

            return actualCastTime;
        }
    };
}
