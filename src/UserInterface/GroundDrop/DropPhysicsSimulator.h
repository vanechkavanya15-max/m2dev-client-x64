#pragma once

#include "IDropPhysicsSimulator.h"

namespace UserInterface::GroundDrop
{
    class DropPhysicsSimulator : public IDropPhysicsSimulator
    {
    public:
        DropPhysicsSimulator() = default;
        ~DropPhysicsSimulator() override = default;

        void SpawnImpulse(uint32_t virtualId, float originX, float originY, float originZ, float force) override;
        void StepPhysics(float deltaTime, float groundHeight) override;
        DropTrajectory GetTrajectory(uint32_t virtualId) const override;
        void Clear() override;
    };
}
