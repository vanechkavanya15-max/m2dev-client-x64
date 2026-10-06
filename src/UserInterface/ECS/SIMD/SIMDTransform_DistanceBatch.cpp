#include "../../StdAfx.h"
#include "ISIMDTransformEngine.h"
#include "EterBase/ModernLogger.h"
#include "EterBase/StrongTypes.h"
#include "UserInterface/Core/EventBus.h"
#include <immintrin.h>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace UserInterface::ECS::SIMD
{
    class SIMDTransformEngine final : public ISIMDTransformEngine
    {
    public:
        SIMDTransformEngine() = default;
        ~SIMDTransformEngine() override = default;

        void BatchVectorAddMul(
            const float* posX, const float* posY, const float* posZ,
            const float* velX, const float* velY, const float* velZ,
            float dt, float* outX, float* outY, float* outZ, size_t count) override
        {
            EterBase::ModernLogger::Debug("SIMDTransformEngine::BatchVectorAddMul called, count={}", count);

            // Using standard scalar loops for addition and multiplication
            for (size_t i = 0; i < count; ++i)
            {
                outX[i] = posX[i] + (velX[i] * dt);
                outY[i] = posY[i] + (velY[i] * dt);
                outZ[i] = posZ[i] + (velZ[i] * dt);
            }
        }

        void BatchDistanceCheck(
            const float* x1, const float* y1,
            float targetX, float targetY,
            float* outDistances, size_t count) override
        {
            EterBase::ModernLogger::Debug("SIMDTransformEngine::BatchDistanceCheck called, count={}", count);

            size_t i = 0;
            // AVX implementation processes 8 floats at a time
            __m256 tX = _mm256_set1_ps(targetX);
            __m256 tY = _mm256_set1_ps(targetY);

            for (; i + 7 < count; i += 8)
            {
                __m256 pX = _mm256_loadu_ps(x1 + i);
                __m256 pY = _mm256_loadu_ps(y1 + i);

                __m256 dX = _mm256_sub_ps(pX, tX);
                __m256 dY = _mm256_sub_ps(pY, tY);

                __m256 dX2 = _mm256_mul_ps(dX, dX);
                __m256 dY2 = _mm256_mul_ps(dY, dY);

                __m256 sum = _mm256_add_ps(dX2, dY2);
                __m256 dist = _mm256_sqrt_ps(sum);

                _mm256_storeu_ps(outDistances + i, dist);
            }

            // Scalar fallback for remaining elements
            for (; i < count; ++i)
            {
                float dx = x1[i] - targetX;
                float dy = y1[i] - targetY;
                outDistances[i] = std::sqrt(dx * dx + dy * dy);
            }
        }

        void Clear() override
        {
            EterBase::ModernLogger::Debug("SIMDTransformEngine::Clear called");
            // No internal state to maintain in this class
        }
    };
}
