#include "../../StdAfx.h"
#include "ISIMDCombatEvaluator.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

#include <immintrin.h>
#include <algorithm>
#include <expected>

namespace UserInterface::ECS::SIMD
{
    /**
     * @brief A SIMD evaluator specifically for calculating HP ratio using AVX2.
     * 
     * Handles 8 units per iteration.
     */
    class SIMDCombatEvaluator_HpRatio final : public ISIMDCombatEvaluator
    {
    public:
        SIMDCombatEvaluator_HpRatio()
        {
            EterBase::ModernLogger::Info("SIMDCombatEvaluator_HpRatio created (AVX2).");
        }

        ~SIMDCombatEvaluator_HpRatio() override = default;

        /**
         * @brief Evaluates alive mask. Stub implementation.
         * @param currentHp Array of current HP.
         * @param outAliveMask Output array for alive mask.
         * @param count Number of elements.
         */
        void EvaluateAliveMask(const uint32_t* currentHp, uint8_t* outAliveMask, size_t count) override
        {
            EterBase::ModernLogger::Debug("EvaluateAliveMask called but not fully implemented in this evaluator.");
            for (size_t i = 0; i < count; ++i)
            {
                outAliveMask[i] = currentHp[i] > 0 ? 1 : 0;
                if (currentHp[i] == 0)
                {
                    // Publishing an event using StrongType and EventBus
                    EterBase::EntityId entityId(static_cast<uint32_t>(i)); 
                    UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::ActorDeadEvent(entityId.value()));
                }
            }
        }

        /**
         * @brief Evaluates the HP ratio (current / max) using AVX2.
         * @param currentHp Array of current HP.
         * @param maxHp Array of max HP.
         * @param outRatio Output array for HP ratios (float).
         * @param count Number of elements.
         */
        void EvaluateHpRatio(const uint32_t* currentHp, const uint32_t* maxHp, float* outRatio, size_t count) override
        {
            if (!currentHp || !maxHp || !outRatio || count == 0)
            {
                EterBase::ModernLogger::Error("Invalid arguments passed to EvaluateHpRatio");
                return;
            }

            EterBase::ModernLogger::Trace("EvaluateHpRatio computing {} entities with AVX2", count);

            size_t i = 0;
            const size_t simdCount = count - (count % 8);
            
            // AVX2 8-element processing
            __m256 zeros = _mm256_setzero_ps();
            __m256 ones = _mm256_set1_ps(1.0f);

            for (; i < simdCount; i += 8)
            {
                // Load 8 uint32_t from currentHp and maxHp
                __m256i curInt = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(currentHp + i));
                __m256i maxInt = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(maxHp + i));

                // Convert to float
                __m256 curFloat = _mm256_cvtepi32_ps(curInt);
                __m256 maxFloat = _mm256_cvtepi32_ps(maxInt);

                // Prevent division by zero: if maxFloat == 0, set to 1.0f
                __m256 cmpMaxZero = _mm256_cmp_ps(maxFloat, zeros, _CMP_EQ_OQ);
                maxFloat = _mm256_blendv_ps(maxFloat, ones, cmpMaxZero);

                // current / max
                __m256 ratio = _mm256_div_ps(curFloat, maxFloat);

                // Store result
                _mm256_storeu_ps(outRatio + i, ratio);
            }

            // Process remainder
            for (; i < count; ++i)
            {
                if (maxHp[i] == 0)
                {
                    outRatio[i] = 0.0f;
                }
                else
                {
                    outRatio[i] = static_cast<float>(currentHp[i]) / static_cast<float>(maxHp[i]);
                }
            }
            
            EterBase::ModernLogger::Debug("EvaluateHpRatio AVX2 computation complete.");
        }

        /**
         * @brief Evaluates aggro range. Stub implementation.
         * @param distances Array of distances.
         * @param aggroRadius The aggro radius threshold.
         * @param outAggroMask Output array for aggro mask.
         * @param count Number of elements.
         */
        void EvaluateAggroRange(const float* distances, float aggroRadius, uint8_t* outAggroMask, size_t count) override
        {
            EterBase::ModernLogger::Debug("EvaluateAggroRange stub called.");
            for (size_t i = 0; i < count; ++i)
            {
                outAggroMask[i] = distances[i] <= aggroRadius ? 1 : 0;
            }
        }

        /**
         * @brief Clears internal state.
         */
        void Clear() override
        {
            EterBase::ModernLogger::Debug("Clear called.");
        }
    };
}
