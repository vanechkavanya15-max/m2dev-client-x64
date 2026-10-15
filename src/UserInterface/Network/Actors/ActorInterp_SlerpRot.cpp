#include "../../StdAfx.h"
#include <cmath>
#include <algorithm>
#include <memory>
#include "IActorInterpolationService.h"
#include "../../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"

namespace UserInterface::Network
{
    struct ActorRotationInterpolatedEvent : public UserInterface::Core::IEvent
    {
        float currentYaw;
        float targetYaw;
        float alpha;
        float resultYaw;

        ActorRotationInterpolatedEvent(float c, float t, float a, float r)
            : currentYaw(c), targetYaw(t), alpha(a), resultYaw(r) {}
    };

    class ActorSlerpRotationService final : public IActorInterpolationService
    {
    public:
        void InterpolateHermite(EterBase::EntityId, float, float, float, float, float) override {}
        void StepDelta(float) override {}
        void Clear() override {}

        float SlerpRotation(float currentYaw, float targetYaw, float alpha) override
        {
            float difference = targetYaw - currentYaw;

            while (difference < -180.0f) difference += 360.0f;
            while (difference > 180.0f) difference -= 360.0f;

            float resultYaw = currentYaw + difference * alpha;

            while (resultYaw < 0.0f) resultYaw += 360.0f;
            while (resultYaw >= 360.0f) resultYaw -= 360.0f;

            EterBase::ModernLogger::Debug("ActorInterpolationService: SlerpRotation applied. Current: {:.2f}, Target: {:.2f}, Alpha: {:.2f}, Result: {:.2f}", 
                                          currentYaw, targetYaw, alpha, resultYaw);

            ActorRotationInterpolatedEvent event(currentYaw, targetYaw, alpha, resultYaw);
            UserInterface::Core::EventBus::GetInstance().Publish(event);

            return resultYaw;
        }
    };

    std::unique_ptr<IActorInterpolationService> CreateActorSlerpRotationService()
    {
        return std::make_unique<ActorSlerpRotationService>();
    }
}
