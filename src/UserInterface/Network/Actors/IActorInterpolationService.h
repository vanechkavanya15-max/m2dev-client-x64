#pragma once

#include <cstdint>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::Network
{
    class IActorInterpolationService
    {
    public:
        virtual ~IActorInterpolationService() = default;

        virtual void InterpolateHermite(EterBase::EntityId id, float startX, float startY, float endX, float endY, float factor) = 0;
        virtual float SlerpRotation(float currentYaw, float targetYaw, float alpha) = 0;
        virtual void StepDelta(float deltaTime) = 0;
        virtual void Clear() = 0;
    };
}
