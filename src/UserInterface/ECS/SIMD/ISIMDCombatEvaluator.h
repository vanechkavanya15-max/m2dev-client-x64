#pragma once

#include <cstdint>
#include <cstddef>

namespace UserInterface::ECS::SIMD
{
    class ISIMDCombatEvaluator
    {
    public:
        virtual ~ISIMDCombatEvaluator() = default;

        virtual void EvaluateAliveMask(
            const uint32_t* currentHp, uint8_t* outAliveMask, size_t count) = 0;

        virtual void EvaluateHpRatio(
            const uint32_t* currentHp, const uint32_t* maxHp,
            float* outRatio, size_t count) = 0;

        virtual void EvaluateAggroRange(
            const float* distances, float aggroRadius,
            uint8_t* outAggroMask, size_t count) = 0;

        virtual void Clear() = 0;
    };
}
