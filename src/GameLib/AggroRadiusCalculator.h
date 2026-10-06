#pragma once

#include <cstdint>
#include <optional>
#include <format>
#include "../EterBase/StrongTypes.h"
#include "../EterBase/Result.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"

namespace CombatMath
{

/**
 * @brief Enum defining actor race types to determine aggro radius multipliers.
 */
enum class RaceType : uint8_t
{
    Normal = 0,
    Boss = 1,
    Huge = 2
};

/**
 * @brief Event published when an aggro radius has been calculated.
 */
struct AggroRadiusCalculatedEvent : public UserInterface::Core::IEvent
{
    EterBase::EntityId entityId;
    float radius;

    /**
     * @brief Constructs the event with calculated radius.
     * @param id The entity ID for which radius was calculated.
     * @param rad The calculated aggro radius.
     */
    AggroRadiusCalculatedEvent(EterBase::EntityId id, float rad)
        : entityId(id), radius(rad) {}
};

/**
 * @brief Request data for calculating an actor's aggro radius.
 */
struct AggroRadiusRequest
{
    EterBase::EntityId entityId;
    EterBase::PlayerLevel level;
    RaceType raceType;
};

/**
 * @brief The AggroRadiusCalculator is responsible for determining the distance 
 * at which an aggressive monster will detect a target.
 *
 * Implements C++23 standards, strong types, and uses the event bus to decouple from GUI.
 */
class AggroRadiusCalculator
{
public:
    /**
     * @brief Base aggro radius for all monsters before multipliers.
     */
    static constexpr float BaseAggroRadius = 1500.0f;

    /**
     * @brief Calculates and publishes the aggro radius for a given request.
     * 
     * @param request The actor data to calculate the radius for.
     * @return EterBase::Result<float, EterBase::EntityError> The calculated radius on success, or an error.
     */
    static EterBase::Result<float, EterBase::EntityError> CalculateAndPublish(const AggroRadiusRequest& request)
    {
        if (!request.entityId)
        {
            EterBase::ModernLogger::Warn("Failed to calculate aggro radius: Invalid EntityId");
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        return CalculateInternal(request)
            .transform([&request](float radius) {
                Core::EventBus::Instance().Publish(AggroRadiusCalculatedEvent{request.entityId, radius});
                EterBase::ModernLogger::Debug(
                    "Calculated aggro radius {:.2f} for entity {} (Level: {}, RaceType: {})", 
                    radius, 
                    request.entityId.get(), 
                    request.level.get(), 
                    static_cast<uint32_t>(request.raceType)
                );
                return radius;
            });
    }

private:
    /**
     * @brief Internal monadic calculation logic.
     * 
     * @param request The actor data to calculate the radius for.
     * @return EterBase::Result<float, EterBase::EntityError> The computed radius.
     */
    static EterBase::Result<float, EterBase::EntityError> CalculateInternal(const AggroRadiusRequest& request)
    {
        float multiplier = 1.0f;
        
        switch (request.raceType)
        {
            case RaceType::Normal:
                multiplier = 1.0f;
                break;
            case RaceType::Boss:
                multiplier = 2.0f;
                break;
            case RaceType::Huge:
                multiplier = 3.0f;
                break;
            default:
                EterBase::ModernLogger::Warn("Unknown race type {}; defaulting to normal multiplier", static_cast<uint32_t>(request.raceType));
                multiplier = 1.0f;
                break;
        }

        // Level-based expansion: Each level adds a small fraction to the radius.
        // Capped at level 120 (max standard level).
        uint8_t effectiveLevel = std::min(request.level.get(), static_cast<uint8_t>(120));
        float levelBonus = static_cast<float>(effectiveLevel) * 5.0f;

        float finalRadius = (BaseAggroRadius * multiplier) + levelBonus;
        return finalRadius;
    }
};

} // namespace CombatMath
