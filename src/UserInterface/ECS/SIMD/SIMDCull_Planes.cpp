#include "../../StdAfx.h"
#include "ISIMDFrustumCuller.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/Result.h"
#include "../../Core/EventBus.h"
#include <immintrin.h>
#include <memory>
#include <cmath>

namespace
{
    using namespace UserInterface::ECS::SIMD;

    struct CullingCompletedEvent : public UserInterface::Core::IEvent
    {
        size_t visibleCount;
        explicit CullingCompletedEvent(size_t count) : visibleCount(count) {}
    };

    class SIMDFrustumCuller : public ISIMDFrustumCuller
    {
    private:
        alignas(32) __m256 m_planesX[6];
        alignas(32) __m256 m_planesY[6];
        alignas(32) __m256 m_planesZ[6];
        alignas(32) __m256 m_planesW[6];
        alignas(32) __m256 m_absPlanesX[6];
        alignas(32) __m256 m_absPlanesY[6];
        alignas(32) __m256 m_absPlanesZ[6];
        
        FrustumPlanes m_scalarPlanes;

    public:
        void SetFrustum(const FrustumPlanes& planes) override
        {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "SIMDFrustumCuller::SetFrustum called");
            
            m_scalarPlanes = planes;
            __m256 zero = _mm256_setzero_ps();
            
            for (int i = 0; i < 6; ++i)
            {
                m_planesX[i] = _mm256_set1_ps(planes.planes[i][0]);
                m_planesY[i] = _mm256_set1_ps(planes.planes[i][1]);
                m_planesZ[i] = _mm256_set1_ps(planes.planes[i][2]);
                m_planesW[i] = _mm256_set1_ps(planes.planes[i][3]);

                // _mm256_max_ps for absolute values: max(x, -x)
                m_absPlanesX[i] = _mm256_max_ps(m_planesX[i], _mm256_sub_ps(zero, m_planesX[i]));
                m_absPlanesY[i] = _mm256_max_ps(m_planesY[i], _mm256_sub_ps(zero, m_planesY[i]));
                m_absPlanesZ[i] = _mm256_max_ps(m_planesZ[i], _mm256_sub_ps(zero, m_planesZ[i]));
            }
        }

        void BatchCullAABB(
            const float* minX, const float* minY, const float* minZ,
            const float* maxX, const float* maxY, const float* maxZ,
            uint8_t* outVisibleMask, size_t count) override
        {
            size_t i = 0;
            const __m256 half = _mm256_set1_ps(0.5f);
            const __m256 zero = _mm256_setzero_ps();

            for (; i + 7 < count; i += 8)
            {
                __m256 xmin = _mm256_loadu_ps(minX + i);
                __m256 ymin = _mm256_loadu_ps(minY + i);
                __m256 zmin = _mm256_loadu_ps(minZ + i);
                
                __m256 xmax = _mm256_loadu_ps(maxX + i);
                __m256 ymax = _mm256_loadu_ps(maxY + i);
                __m256 zmax = _mm256_loadu_ps(maxZ + i);

                __m256 cx = _mm256_mul_ps(_mm256_add_ps(xmax, xmin), half);
                __m256 cy = _mm256_mul_ps(_mm256_add_ps(ymax, ymin), half);
                __m256 cz = _mm256_mul_ps(_mm256_add_ps(zmax, zmin), half);

                __m256 ex = _mm256_mul_ps(_mm256_sub_ps(xmax, xmin), half);
                __m256 ey = _mm256_mul_ps(_mm256_sub_ps(ymax, ymin), half);
                __m256 ez = _mm256_mul_ps(_mm256_sub_ps(zmax, zmin), half);

                __m256 visible = _mm256_cmp_ps(zero, zero, _CMP_EQ_OQ); // All 1s mask

                for (int p = 0; p < 6; ++p)
                {
                    __m256 d = _mm256_add_ps(
                        _mm256_add_ps(_mm256_mul_ps(cx, m_planesX[p]), _mm256_mul_ps(cy, m_planesY[p])),
                        _mm256_add_ps(_mm256_mul_ps(cz, m_planesZ[p]), m_planesW[p])
                    );

                    __m256 r = _mm256_add_ps(
                        _mm256_add_ps(_mm256_mul_ps(ex, m_absPlanesX[p]), _mm256_mul_ps(ey, m_absPlanesY[p])),
                        _mm256_mul_ps(ez, m_absPlanesZ[p])
                    );

                    // insidePlane is true if d + r >= 0
                    __m256 insidePlane = _mm256_cmp_ps(_mm256_add_ps(d, r), zero, _CMP_GE_OQ);
                    visible = _mm256_and_ps(visible, insidePlane);
                }

                int mask = _mm256_movemask_ps(visible);
                for (int j = 0; j < 8; ++j)
                {
                    outVisibleMask[i + j] = (mask & (1 << j)) ? 1 : 0;
                }
            }

            // Scalar fallback for remaining items
            for (; i < count; ++i)
            {
                float cx = (maxX[i] + minX[i]) * 0.5f;
                float cy = (maxY[i] + minY[i]) * 0.5f;
                float cz = (maxZ[i] + minZ[i]) * 0.5f;
                
                float ex = (maxX[i] - minX[i]) * 0.5f;
                float ey = (maxY[i] - minY[i]) * 0.5f;
                float ez = (maxZ[i] - minZ[i]) * 0.5f;

                uint8_t isVisible = 1;
                for (int p = 0; p < 6; ++p)
                {
                    float d = cx * m_scalarPlanes.planes[p][0] + 
                              cy * m_scalarPlanes.planes[p][1] + 
                              cz * m_scalarPlanes.planes[p][2] + 
                              m_scalarPlanes.planes[p][3];
                    float r = ex * std::abs(m_scalarPlanes.planes[p][0]) + 
                              ey * std::abs(m_scalarPlanes.planes[p][1]) + 
                              ez * std::abs(m_scalarPlanes.planes[p][2]);
                    
                    if (d + r < 0.0f)
                    {
                        isVisible = 0;
                        break;
                    }
                }
                outVisibleMask[i] = isVisible;
            }
        }

        size_t PackVisibleIndices(const uint8_t* visibleMask, size_t count, uint32_t* outIndices) override
        {
            size_t visibleCount = 0;
            for (size_t i = 0; i < count; ++i)
            {
                if (visibleMask[i])
                {
                    outIndices[visibleCount++] = static_cast<uint32_t>(i);
                }
            }
            
            EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "Culling complete, {} visible out of {}", visibleCount, count);

            CullingCompletedEvent event(visibleCount);
            UserInterface::Core::EventBus::GetInstance().Publish(event);

            return visibleCount;
        }

        void Clear() override
        {
            __m256 zero = _mm256_setzero_ps();
            for (int i = 0; i < 6; ++i)
            {
                m_planesX[i] = zero;
                m_planesY[i] = zero;
                m_planesZ[i] = zero;
                m_planesW[i] = zero;
                m_absPlanesX[i] = zero;
                m_absPlanesY[i] = zero;
                m_absPlanesZ[i] = zero;
            }
            
            for (int i = 0; i < 6; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    m_scalarPlanes.planes[i][j] = 0.0f;
                }
            }
        }
    };
}

namespace UserInterface::ECS::SIMD
{
    std::unique_ptr<ISIMDFrustumCuller> CreateSIMDFrustumCuller()
    {
        return std::make_unique<SIMDFrustumCuller>();
    }
}
