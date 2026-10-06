#pragma once

#include <cstdint>
#include "EterBase/Result.h"

namespace UserInterface::GroundDrop
{
    struct DropTrajectory
    {
        float posX{0.0f};
        float posY{0.0f};
        float posZ{0.0f};
        float velX{0.0f};
        float velY{0.0f};
        float velZ{0.0f};
        bool isSettled{false};
    };

    class IDropPhysicsSimulator
    {
    public:
        virtual ~IDropPhysicsSimulator() = default;

        virtual void SpawnImpulse(uint32_t virtualId, float originX, float originY, float originZ, float force) = 0;
        virtual void StepPhysics(float deltaTime, float groundHeight) = 0;
        virtual DropTrajectory GetTrajectory(uint32_t virtualId) const = 0;
        virtual void Clear() = 0;
    };
}
