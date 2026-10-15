#include "../../StdAfx.h"
#include "IActorInterpolationService.h"
#include "../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include "../../EterBase/Result.h"
#include <span>
#include <memory>

namespace UserInterface::Network
{
    /**
     * @brief Event emitted when actor interpolation states are cleared.
     */
    struct ActorInterpolationClearedEvent : public UserInterface::Core::IEvent
    {
    };

    /**
     * @brief Clears the interpolation state and notifies subsystems.
     */
    class ActorClearInterpolationService final : public IActorInterpolationService
    {
    public:
        void InterpolateHermite(EterBase::EntityId, float, float, float, float, float) override {}
        float SlerpRotation(float currentYaw, float, float) override { return currentYaw; }
        void StepDelta(float) override {}

        void Clear() override
        {
            EterBase::ModernLogger::Info("ActorClearInterpolationService: Clearing interpolation states.");
            ActorInterpolationClearedEvent eventPayload;
            UserInterface::Core::EventBus::GetInstance().Publish(eventPayload);
        }

        EterBase::PacketResult<void> HandleClear(std::span<const uint8_t> /*buffer*/)
        {
            Clear();
            return {};
        }
    };

    std::unique_ptr<IActorInterpolationService> CreateActorClearInterpolationService()
    {
        return std::make_unique<ActorClearInterpolationService>();
    }
}
