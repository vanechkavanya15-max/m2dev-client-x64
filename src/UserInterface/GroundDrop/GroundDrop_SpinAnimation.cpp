#include "../StdAfx.h"
#include <cmath>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

namespace UserInterface::GroundDrop::SpinAnimation
{
    /**
     * @brief Custom event triggered when a drop's spin rotation is updated.
     */
    struct DropSpinUpdatedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId virtualId;
        float newRotation;

        /**
         * @brief Constructs the DropSpinUpdatedEvent.
         * @param id The virtual ID of the drop.
         * @param rot The newly calculated rotation in degrees.
         */
        explicit DropSpinUpdatedEvent(EterBase::EntityId id, float rot)
            : virtualId(id), newRotation(rot) {}
    };

    /**
     * @brief Updates the Z-axis rotation of a ground drop for a spin animation.
     * @param virtualId The virtual ID of the item.
     * @param currentRotation The current rotation in degrees.
     * @param deltaTime The time elapsed since the last update.
     * @param spinSpeed The speed of the spin in degrees per second.
     * @return Result containing the new rotation, or EntityError if invalid parameters.
     */
    EterBase::Result<float, EterBase::EntityError> UpdateSpin(EterBase::EntityId virtualId, float currentRotation, float deltaTime, float spinSpeed)
    {
        if (deltaTime < 0.0f)
        {
            EterBase::ModernLogger::Warn("Invalid deltaTime {} for spin update on entity {}", deltaTime, virtualId.value());
            return std::unexpected(EterBase::EntityError::InvalidType);
        }

        float newRotation = currentRotation + (spinSpeed * deltaTime);

        // Normalize rotation to [0, 360)
        newRotation = std::fmod(newRotation, 360.0f);
        if (newRotation < 0.0f)
        {
            newRotation += 360.0f;
        }

        EterBase::ModernLogger::Trace("UpdateSpin: Entity {} new rotation: {}", virtualId.value(), newRotation);

        DropSpinUpdatedEvent event(virtualId, newRotation);
        UserInterface::Core::EventBus::GetInstance().Publish(event);

        return newRotation;
    }
}
