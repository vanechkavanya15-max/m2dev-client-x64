#pragma once

#include <cstdint>
#include <string_view>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::InstanceControllers
{
    enum class MotionState : uint8_t
    {
        Idle = 0,
        Walk,
        Run,
        Attack,
        Skill,
        Damaged,
        Dead
    };

    struct MotionConfig
    {
        uint32_t motionKey{0};
        float blendTime{0.15f};
        float speedMultiplier{1.0f};
        bool isLooping{false};
    };

    class IInstanceAnimationController
    {
    public:
        virtual ~IInstanceAnimationController() = default;

        virtual EterBase::PacketResult<void> PlayMotion(const MotionConfig& config) = 0;
        virtual EterBase::PacketResult<void> BlendMotion(uint32_t motionKey, float blendDuration) = 0;
        virtual void SetMotionSpeed(float multiplier) = 0;
        virtual bool IsMotionFinished() const = 0;
        virtual void CancelMotion() = 0;
        virtual uint32_t GetCurrentMotion() const = 0;
        virtual MotionState GetMotionState() const = 0;
        virtual void Clear() = 0;
    };
}
