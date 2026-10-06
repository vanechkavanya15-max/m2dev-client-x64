#include "../StdAfx.h"
#include "IInstanceAnimationController.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "../Core/EventBus.h"
#include <format>
#include <expected>

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Event published when an animation is cancelled due to stun, knockback, or death.
     */
    struct MotionCancelledEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        uint32_t previousMotionKey{0};
        MotionState previousState{MotionState::Idle};
    };

    /**
     * @brief Enumeration of reasons to cancel an animation.
     */
    enum class CancelReason : uint8_t
    {
        Stun = 0,
        Knockback,
        Death
    };

    /**
     * @brief Strategy module for handling animation cancellations securely.
     * 
     * Complies with Single Responsibility Principle and Zero-Conflict by not inheriting
     * from IInstanceAnimationController, but rather operating on it.
     */
    class InstanceAnim_Cancel
    {
    public:
        /**
         * @brief Constructs the cancellation controller.
         * @param entityId The ID of the entity whose animation is being managed.
         * @param controller Pointer to the main animation controller.
         */
        InstanceAnim_Cancel(EterBase::EntityId entityId, IInstanceAnimationController* controller) 
            : entityId_(entityId), controller_(controller) 
        {
        }

        /**
         * @brief Cancels the current motion immediately upon specified reasons.
         * @param reason The reason for cancellation (Stun, Knockback, Death).
         * @return EterBase::PacketResult<void> representing success or error state.
         */
        EterBase::PacketResult<void> CancelMotionOnEvent(CancelReason reason)
        {
            if (!controller_)
            {
                EterBase::ModernLogger::Error("InstanceAnim_Cancel::CancelMotionOnEvent - Null animation controller provided");
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            EterBase::ModernLogger::Info("InstanceAnim_Cancel::CancelMotionOnEvent - Cancelling motion for EntityId: {}, Reason: {}", entityId_.value(), static_cast<uint8_t>(reason));

            MotionCancelledEvent event;
            event.entityId = entityId_;
            event.previousMotionKey = controller_->GetCurrentMotion();
            event.previousState = controller_->GetMotionState();

            // Perform the cancellation on the underlying controller
            controller_->CancelMotion();

            // Publish the event to decouple GUI/System interactions
            Core::EventBus::GetInstance().Publish(event);

            return {};
        }

    private:
        EterBase::EntityId entityId_;
        IInstanceAnimationController* controller_{nullptr};
    };
}
