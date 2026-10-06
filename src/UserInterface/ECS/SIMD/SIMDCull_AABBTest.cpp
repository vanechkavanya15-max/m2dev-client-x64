#include "../../StdAfx.h"
#include "ISIMDFrustumCuller.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

#include <immintrin.h>
#include <algorithm>
#include <cstring>
#include <format>
#include <expected>

namespace {
    struct FrustumUpdateEvent : public UserInterface::Core::IEvent {
        // Event broadcasted when frustum planes are updated.
    };
    
    struct FrustumCullCompleteEvent : public UserInterface::Core::IEvent {
        size_t processedCount;
        size_t visibleCount;
    };
}

namespace UserInterface::ECS::SIMD {

class SIMDFrustumCuller_AABBTest final : public ISIMDFrustumCuller {
private:
    alignas(32) float m_planes[6][4];
    bool m_hasFrustum = false;

public:
    SIMDFrustumCuller_AABBTest() {
        Clear();
    }

    ~SIMDFrustumCuller_AABBTest() override = default;

    void SetFrustum(const FrustumPlanes& planes) override {
        for (int i = 0; i < 6; ++i) {
            for (int j = 0; j < 4; ++j) {
                m_planes[i][j] = planes.planes[i][j];
            }
        }
        m_hasFrustum = true;

        EterBase::ModernLogger::Info("SIMDFrustumCuller: Frustum planes updated.");
        
        FrustumUpdateEvent ev;
        UserInterface::Core::EventBus::GetInstance().Publish(ev);
    }

    void BatchCullAABB(
        const float* minX, const float* minY, const float* minZ,
        const float* maxX, const float* maxY, const float* maxZ,
        uint8_t* outVisibleMask, size_t count) override 
    {
        if (!m_hasFrustum) {
            EterBase::ModernLogger::Warn("SIMDFrustumCuller: BatchCullAABB called before SetFrustum. Marking all as visible.");
            std::memset(outVisibleMask, 1, count);
            return;
        }

        size_t i = 0;
        
        // Setup planes in SIMD format
        __m256 pX[6], pY[6], pZ[6], pD[6];
        for (int p = 0; p < 6; ++p) {
            pX[p] = _mm256_set1_ps(m_planes[p][0]);
            pY[p] = _mm256_set1_ps(m_planes[p][1]);
            pZ[p] = _mm256_set1_ps(m_planes[p][2]);
            pD[p] = _mm256_set1_ps(m_planes[p][3]);
        }
        
        const __m256 vZero = _mm256_setzero_ps();

        // 8-way parallel AABB tests
        for (; i + 7 < count; i += 8) {
            __m256 vMinX = _mm256_loadu_ps(minX + i);
            __m256 vMinY = _mm256_loadu_ps(minY + i);
            __m256 vMinZ = _mm256_loadu_ps(minZ + i);
            __m256 vMaxX = _mm256_loadu_ps(maxX + i);
            __m256 vMaxY = _mm256_loadu_ps(maxY + i);
            __m256 vMaxZ = _mm256_loadu_ps(maxZ + i);

            // Start with all bits set (1) for visible
            __m256 vInsideMask = _mm256_castsi256_ps(_mm256_set1_epi32(-1));

            for (int p = 0; p < 6; ++p) {
                // Find p-vertex (vertex furthest in direction of normal) for each dimension
                __m256 nx_minX = _mm256_mul_ps(pX[p], vMinX);
                __m256 nx_maxX = _mm256_mul_ps(pX[p], vMaxX);
                __m256 pVx = _mm256_max_ps(nx_minX, nx_maxX);
                
                __m256 ny_minY = _mm256_mul_ps(pY[p], vMinY);
                __m256 ny_maxY = _mm256_mul_ps(pY[p], vMaxY);
                __m256 pVy = _mm256_max_ps(ny_minY, ny_maxY);
                
                __m256 nz_minZ = _mm256_mul_ps(pZ[p], vMinZ);
                __m256 nz_maxZ = _mm256_mul_ps(pZ[p], vMaxZ);
                __m256 pVz = _mm256_max_ps(nz_minZ, nz_maxZ);
                
                // distance = pVx + pVy + pVz + plane_D
                __m256 dist = _mm256_add_ps(_mm256_add_ps(pVx, pVy), _mm256_add_ps(pVz, pD[p]));
                
                // If dist >= 0, it's inside or intersecting the frustum
                __m256 planeMask = _mm256_cmp_ps(dist, vZero, _CMP_GE_OQ);
                
                vInsideMask = _mm256_and_ps(vInsideMask, planeMask);
            }
            
            // Extract the high bits to a single integer mask
            int mask = _mm256_movemask_ps(vInsideMask);
            
            outVisibleMask[i + 0] = (mask >> 0) & 1;
            outVisibleMask[i + 1] = (mask >> 1) & 1;
            outVisibleMask[i + 2] = (mask >> 2) & 1;
            outVisibleMask[i + 3] = (mask >> 3) & 1;
            outVisibleMask[i + 4] = (mask >> 4) & 1;
            outVisibleMask[i + 5] = (mask >> 5) & 1;
            outVisibleMask[i + 6] = (mask >> 6) & 1;
            outVisibleMask[i + 7] = (mask >> 7) & 1;
        }

        // Process remaining elements sequentially
        for (; i < count; ++i) {
            bool inside = true;
            for (int p = 0; p < 6; ++p) {
                float pVx = std::max(m_planes[p][0] * minX[i], m_planes[p][0] * maxX[i]);
                float pVy = std::max(m_planes[p][1] * minY[i], m_planes[p][1] * maxY[i]);
                float pVz = std::max(m_planes[p][2] * minZ[i], m_planes[p][2] * maxZ[i]);
                float dist = pVx + pVy + pVz + m_planes[p][3];
                
                if (dist < 0.0f) {
                    inside = false;
                    break;
                }
            }
            outVisibleMask[i] = inside ? 1 : 0;
        }
    }

    size_t PackVisibleIndices(
        const uint8_t* visibleMask, size_t count, uint32_t* outIndices) override 
    {
        size_t visibleCount = 0;
        for (size_t i = 0; i < count; ++i) {
            if (visibleMask[i]) {
                outIndices[visibleCount++] = static_cast<uint32_t>(i);
            }
        }
        
        FrustumCullCompleteEvent ev;
        ev.processedCount = count;
        ev.visibleCount = visibleCount;
        UserInterface::Core::EventBus::GetInstance().Publish(ev);
        
        return visibleCount;
    }

    void Clear() override {
        std::memset(m_planes, 0, sizeof(m_planes));
        m_hasFrustum = false;
        EterBase::ModernLogger::Debug("SIMDFrustumCuller: Cleared state.");
    }
};

} // namespace UserInterface::ECS::SIMD
