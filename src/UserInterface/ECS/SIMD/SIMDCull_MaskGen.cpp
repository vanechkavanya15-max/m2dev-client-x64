#include "../../StdAfx.h"
#include "ISIMDFrustumCuller.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "Core/EventBus.h"

#include <immintrin.h>
#include <memory>
#include <expected>
#include <algorithm>

namespace UserInterface::ECS::SIMD
{
    /**
     * @brief Zdarzenie emitowane po zakonczeniu culling'u frustum w paczce.
     */
    struct FrustumCullCompletedEvent final : public Core::IEvent
    {
        size_t processedCount;
        size_t visibleCount;

        FrustumCullCompletedEvent(size_t pCount, size_t vCount)
            : processedCount(pCount), visibleCount(vCount) {}
    };

    /**
     * @brief Implementacja generatora maski widocznosci przy uzyciu intrynsek AVX.
     */
    class SIMDFrustumCuller final : public ISIMDFrustumCuller
    {
    public:
        void SetFrustum(const FrustumPlanes& planes) override
        {
            m_planes = planes;
            m_hasFrustum = true;
            EterBase::ModernLogger::Debug("SIMDFrustumCuller: Frustum planes configured");
        }

        void BatchCullAABB(
            const float* minX, const float* minY, const float* minZ,
            const float* maxX, const float* maxY, const float* maxZ,
            uint8_t* outVisibleMask, size_t count) override
        {
            if (!m_hasFrustum || count == 0)
            {
                return;
            }

            // Inicjalizacja maski - zakladamy, ze encje sa poczatkowo widoczne
            for (size_t i = 0; i < count; ++i)
            {
                outVisibleMask[i] = 1;
            }

            // Przetwarzamy po plaszczyznach
            for (int p = 0; p < 6; ++p)
            {
                const float nx = m_planes.planes[p][0];
                const float ny = m_planes.planes[p][1];
                const float nz = m_planes.planes[p][2];
                const float d  = m_planes.planes[p][3];

                const float* pxArray = (nx > 0.0f) ? maxX : minX;
                const float* pyArray = (ny > 0.0f) ? maxY : minY;
                const float* pzArray = (nz > 0.0f) ? maxZ : minZ;

                // SIMD AVX: przetwarzamy paczki po 8
                size_t i = 0;
                const size_t simdCount = count & ~7; // floor do 8

                __m256 vNx = _mm256_set1_ps(nx);
                __m256 vNy = _mm256_set1_ps(ny);
                __m256 vNz = _mm256_set1_ps(nz);
                __m256 vD  = _mm256_set1_ps(d);
                __m256 vZero = _mm256_setzero_ps();

                for (; i < simdCount; i += 8)
                {
                    __m256 vPx = _mm256_loadu_ps(&pxArray[i]);
                    __m256 vPy = _mm256_loadu_ps(&pyArray[i]);
                    __m256 vPz = _mm256_loadu_ps(&pzArray[i]);

                    __m256 dist = _mm256_add_ps(
                        _mm256_add_ps(
                            _mm256_add_ps(_mm256_mul_ps(vNx, vPx), _mm256_mul_ps(vNy, vPy)),
                            _mm256_mul_ps(vNz, vPz)
                        ),
                        vD
                    );

                    // Porownanie dist < 0.0f
                    __m256 cmp = _mm256_cmp_ps(dist, vZero, _CMP_LT_OQ);

                    // Pobranie 8-bitowej maski z porownania (1 bit na encje)
                    int mask = _mm256_movemask_ps(cmp);

                    // Aplikowanie maski jezeli ktores byly na zewnatrz
                    if (mask != 0)
                    {
                        for (int bit = 0; bit < 8; ++bit)
                        {
                            if ((mask & (1 << bit)) != 0)
                            {
                                outVisibleMask[i + bit] = 0;
                            }
                        }
                    }
                }

                // Reszta
                for (; i < count; ++i)
                {
                    const float dist = nx * pxArray[i] + ny * pyArray[i] + nz * pzArray[i] + d;
                    if (dist < 0.0f)
                    {
                        outVisibleMask[i] = 0;
                    }
                }
            }

            size_t visibleCount = 0;
            for (size_t i = 0; i < count; ++i)
            {
                visibleCount += outVisibleMask[i];
            }

            Core::EventBus::GetInstance().Publish(FrustumCullCompletedEvent(count, visibleCount));
        }

        size_t PackVisibleIndices(
            const uint8_t* visibleMask, size_t count, uint32_t* outIndices) override
        {
            size_t outCount = 0;
            for (size_t i = 0; i < count; ++i)
            {
                if (visibleMask[i])
                {
                    outIndices[outCount++] = static_cast<uint32_t>(i);
                }
            }
            return outCount;
        }

        void Clear() override
        {
            m_hasFrustum = false;
            EterBase::ModernLogger::Debug("SIMDFrustumCuller: State cleared");
        }

    private:
        FrustumPlanes m_planes{};
        bool m_hasFrustum = false;
    };

    /**
     * @brief Zwraca nowa instancje kontrolera Frustum culling'u.
     */
    std::expected<std::unique_ptr<ISIMDFrustumCuller>, EterBase::EntityError> CreateSIMDFrustumCuller()
    {
        EterBase::ModernLogger::Info("SIMDFrustumCuller instance created successfully");
        return std::make_unique<SIMDFrustumCuller>();
    }
}
