#include "../../StdAfx.h"
#include "ISIMDCombatEvaluator.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

#include <immintrin.h>
#include <memory>
#include <cstring>

namespace UserInterface::Core
{
    struct AggroCheckCompletedEvent : public IEvent
    {
        size_t processedCount;
        float aggroRadius;

        AggroCheckCompletedEvent(size_t count, float radius)
            : processedCount(count), aggroRadius(radius) {}
    };
}

namespace UserInterface::ECS::SIMD
{

class SIMDCombatEvaluator : public ISIMDCombatEvaluator
{
public:
    SIMDCombatEvaluator()
    {
        EterBase::ModernLogger::Info("SIMDCombatEvaluator initialized. Prepared for AVX vectorization.");
    }

    ~SIMDCombatEvaluator() override
    {
        Clear();
    }

    void EvaluateAliveMask(const uint32_t* currentHp, uint8_t* outAliveMask, size_t count) override
    {
        if (!currentHp || !outAliveMask)
        {
            EterBase::ModernLogger::Error("EvaluateAliveMask: Null pointer provided.");
            return;
        }

        size_t i = 0;
#if defined(__AVX2__)
        __m256i zero = _mm256_setzero_si256();
        for (; i + 8 <= count; i += 8)
        {
            __m256i hp = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(currentHp + i));
            __m256i mask = _mm256_cmpgt_epi32(hp, zero);
            
            int bitmask = _mm256_movemask_ps(_mm256_castsi256_ps(mask));
            
            outAliveMask[i + 0] = (bitmask & (1 << 0)) ? 1 : 0;
            outAliveMask[i + 1] = (bitmask & (1 << 1)) ? 1 : 0;
            outAliveMask[i + 2] = (bitmask & (1 << 2)) ? 1 : 0;
            outAliveMask[i + 3] = (bitmask & (1 << 3)) ? 1 : 0;
            outAliveMask[i + 4] = (bitmask & (1 << 4)) ? 1 : 0;
            outAliveMask[i + 5] = (bitmask & (1 << 5)) ? 1 : 0;
            outAliveMask[i + 6] = (bitmask & (1 << 6)) ? 1 : 0;
            outAliveMask[i + 7] = (bitmask & (1 << 7)) ? 1 : 0;
        }
#endif
        for (; i < count; ++i)
        {
            outAliveMask[i] = (currentHp[i] > 0) ? 1 : 0;
        }
    }

    void EvaluateHpRatio(const uint32_t* currentHp, const uint32_t* maxHp, float* outRatio, size_t count) override
    {
        if (!currentHp || !maxHp || !outRatio)
        {
            EterBase::ModernLogger::Error("EvaluateHpRatio: Null pointer provided.");
            return;
        }

        size_t i = 0;
#if defined(__AVX2__)
        __m256 zero = _mm256_setzero_ps();
        for (; i + 8 <= count; i += 8)
        {
            __m256 hp = _mm256_cvtepi32_ps(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(currentHp + i)));
            __m256 mhp = _mm256_cvtepi32_ps(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(maxHp + i)));
            
            // Avoid division by zero: check where maxHp > 0
            __m256 mask = _mm256_cmp_ps(mhp, zero, _CMP_GT_OQ);
            
            // Calculate division, default to 0 for invalid values
            __m256 ratio = _mm256_div_ps(hp, mhp);
            __m256 safeRatio = _mm256_blendv_ps(zero, ratio, mask);
            
            _mm256_storeu_ps(outRatio + i, safeRatio);
        }
#endif
        for (; i < count; ++i)
        {
            outRatio[i] = maxHp[i] > 0 ? static_cast<float>(currentHp[i]) / static_cast<float>(maxHp[i]) : 0.0f;
        }
    }

    void EvaluateAggroRange(const float* distances, float aggroRadius, uint8_t* outAggroMask, size_t count) override
    {
        if (!distances || !outAggroMask)
        {
            EterBase::ModernLogger::Error("EvaluateAggroRange: Null pointer provided.");
            return;
        }

        size_t i = 0;
#if defined(__AVX__)
        __m256 radius = _mm256_set1_ps(aggroRadius);
        for (; i + 8 <= count; i += 8)
        {
            __m256 dist = _mm256_loadu_ps(distances + i);
            __m256 mask = _mm256_cmp_ps(dist, radius, _CMP_LE_OQ);
            
            int bitmask = _mm256_movemask_ps(mask);
            
            outAggroMask[i + 0] = (bitmask & (1 << 0)) ? 1 : 0;
            outAggroMask[i + 1] = (bitmask & (1 << 1)) ? 1 : 0;
            outAggroMask[i + 2] = (bitmask & (1 << 2)) ? 1 : 0;
            outAggroMask[i + 3] = (bitmask & (1 << 3)) ? 1 : 0;
            outAggroMask[i + 4] = (bitmask & (1 << 4)) ? 1 : 0;
            outAggroMask[i + 5] = (bitmask & (1 << 5)) ? 1 : 0;
            outAggroMask[i + 6] = (bitmask & (1 << 6)) ? 1 : 0;
            outAggroMask[i + 7] = (bitmask & (1 << 7)) ? 1 : 0;
        }
#endif
        for (; i < count; ++i)
        {
            outAggroMask[i] = (distances[i] <= aggroRadius) ? 1 : 0;
        }

        // Powiadomienie innych podsystemow o zakonczeniu kalkulacji agresji.
        UserInterface::Core::EventBus::GetInstance().Publish(
            UserInterface::Core::AggroCheckCompletedEvent(count, aggroRadius)
        );
    }

    void Clear() override
    {
        EterBase::ModernLogger::Debug("SIMDCombatEvaluator: State cleared.");
    }
};

std::unique_ptr<ISIMDCombatEvaluator> CreateSIMDCombatEvaluator()
{
    return std::make_unique<SIMDCombatEvaluator>();
}

} // namespace UserInterface::ECS::SIMD
