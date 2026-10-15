#include "../../StdAfx.h"
#include "ISIMDTransformEngine.h"
#include "EterBase/LogModern.h"
#include <immintrin.h>
#include <cmath>

namespace UserInterface::ECS::SIMD
{
    class SIMDTransformEngine final : public ISIMDTransformEngine
    {
    public:
        SIMDTransformEngine()
        {
            EterBase::ModernLogger::Info("SIMDTransformEngine: AVX2 engine initialized.");
        }

        ~SIMDTransformEngine() override
        {
            EterBase::ModernLogger::Info("SIMDTransformEngine: shut down.");
        }

        void BatchVectorAddMul(
            const float* posX, const float* posY, const float* posZ,
            const float* velX, const float* velY, const float* velZ,
            float dt, float* outX, float* outY, float* outZ, size_t count) override
        {
            if (!posX || !posY || !posZ || !velX || !velY || !velZ || !outX || !outY || !outZ)
            {
                EterBase::ModernLogger::Error("SIMDTransformEngine::BatchVectorAddMul: null pointer passed.");
                return;
            }

            const __m256 v_dt = _mm256_set1_ps(dt);
            size_t i = 0;

            // AVX2 Loop for 8 elements at a time
            for (; i + 7 < count; i += 8)
            {
                __m256 v_posX = _mm256_loadu_ps(&posX[i]);
                __m256 v_posY = _mm256_loadu_ps(&posY[i]);
                __m256 v_posZ = _mm256_loadu_ps(&posZ[i]);

                __m256 v_velX = _mm256_loadu_ps(&velX[i]);
                __m256 v_velY = _mm256_loadu_ps(&velY[i]);
                __m256 v_velZ = _mm256_loadu_ps(&velZ[i]);

                // out = pos + vel * dt
                __m256 v_outX = _mm256_fmadd_ps(v_velX, v_dt, v_posX);
                __m256 v_outY = _mm256_fmadd_ps(v_velY, v_dt, v_posY);
                __m256 v_outZ = _mm256_fmadd_ps(v_velZ, v_dt, v_posZ);

                _mm256_storeu_ps(&outX[i], v_outX);
                _mm256_storeu_ps(&outY[i], v_outY);
                _mm256_storeu_ps(&outZ[i], v_outZ);
            }

            // Scalar fallback for remaining elements
            for (; i < count; ++i)
            {
                outX[i] = posX[i] + velX[i] * dt;
                outY[i] = posY[i] + velY[i] * dt;
                outZ[i] = posZ[i] + velZ[i] * dt;
            }
        }

        void BatchDistanceCheck(
            const float* x1, const float* y1,
            float targetX, float targetY,
            float* outDistances, size_t count) override
        {
            if (!x1 || !y1 || !outDistances)
            {
                EterBase::ModernLogger::Error("SIMDTransformEngine::BatchDistanceCheck: null pointer passed.");
                return;
            }

            const __m256 v_targetX = _mm256_set1_ps(targetX);
            const __m256 v_targetY = _mm256_set1_ps(targetY);
            size_t i = 0;

            // AVX2 Loop for 8 elements at a time
            for (; i + 7 < count; i += 8)
            {
                __m256 v_x1 = _mm256_loadu_ps(&x1[i]);
                __m256 v_y1 = _mm256_loadu_ps(&y1[i]);

                __m256 v_dx = _mm256_sub_ps(v_x1, v_targetX);
                __m256 v_dy = _mm256_sub_ps(v_y1, v_targetY);

                // dx^2 + dy^2
                __m256 v_dx2 = _mm256_mul_ps(v_dx, v_dx);
                __m256 v_distSq = _mm256_fmadd_ps(v_dy, v_dy, v_dx2);

                // sqrt(dx^2 + dy^2)
                __m256 v_dist = _mm256_sqrt_ps(v_distSq);

                _mm256_storeu_ps(&outDistances[i], v_dist);
            }

            // Scalar fallback
            for (; i < count; ++i)
            {
                float dx = x1[i] - targetX;
                float dy = y1[i] - targetY;
                outDistances[i] = std::sqrt(dx * dx + dy * dy);
            }
        }

        void Clear() override
        {
            EterBase::ModernLogger::Debug("SIMDTransformEngine::Clear called.");
        }
    };
}
