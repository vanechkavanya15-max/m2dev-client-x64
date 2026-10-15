#include "../../StdAfx.h"
#include "ISIMDCombatEvaluator.h"
#include "src/EterBase/LogModern.h"
#include "src/EterBase/Result.h"
#include "src/EterBase/StrongTypes.h"
#include "src/UserInterface/Core/EventBus.h"

namespace UserInterface::ECS::SIMD
{
    class SIMDCombatEvaluator : public ISIMDCombatEvaluator
    {
    public:
        void EvaluateAliveMask(
            const uint32_t* currentHp, uint8_t* outAliveMask, size_t count) override
        {
            if (!currentHp || !outAliveMask)
            {
                EterBase::ModernLogger::Error("EvaluateAliveMask: Null pointer provided.");
                return;
            }

            for (size_t i = 0; i < count; ++i)
            {
                outAliveMask[i] = (currentHp[i] > 0) ? 1 : 0;
            }
            EterBase::ModernLogger::Trace("EvaluateAliveMask: Evaluated {} entities.", count);
        }

        void EvaluateHpRatio(
            const uint32_t* currentHp, const uint32_t* maxHp,
            float* outRatio, size_t count) override
        {
            if (!currentHp || !maxHp || !outRatio)
            {
                EterBase::ModernLogger::Error("EvaluateHpRatio: Null pointer provided.");
                return;
            }

            for (size_t i = 0; i < count; ++i)
            {
                if (maxHp[i] > 0)
                {
                    outRatio[i] = static_cast<float>(currentHp[i]) / static_cast<float>(maxHp[i]);
                }
                else
                {
                    outRatio[i] = 0.0f;
                }
            }
            EterBase::ModernLogger::Trace("EvaluateHpRatio: Evaluated {} entities.", count);
        }

        void EvaluateAggroRange(
            const float* distances, float aggroRadius,
            uint8_t* outAggroMask, size_t count) override
        {
            if (!distances || !outAggroMask)
            {
                EterBase::ModernLogger::Error("EvaluateAggroRange: Null pointer provided.");
                return;
            }

            for (size_t i = 0; i < count; ++i)
            {
                outAggroMask[i] = (distances[i] <= aggroRadius) ? 1 : 0;
            }
            EterBase::ModernLogger::Trace("EvaluateAggroRange: Evaluated {} entities with aggro radius {}.", count, aggroRadius);
        }

        void Clear() override
        {
            EterBase::ModernLogger::Info("SIMDCombatEvaluator cleared");
        }
    };
}
