#pragma once

#include <cstdint>
#include <span>
#include "EterBase/Result.h"

namespace UserInterface::ECS::SIMD
{
    class ISIMDTransformEngine
    {
    public:
        virtual ~ISIMDTransformEngine() = default;

        virtual void BatchVectorAddMul(
            const float* posX, const float* posY, const float* posZ,
            const float* velX, const float* velY, const float* velZ,
            float dt, float* outX, float* outY, float* outZ, size_t count) = 0;

        virtual void BatchDistanceCheck(
            const float* x1, const float* y1,
            float targetX, float targetY,
            float* outDistances, size_t count) = 0;

        virtual void Clear() = 0;
    };
}
