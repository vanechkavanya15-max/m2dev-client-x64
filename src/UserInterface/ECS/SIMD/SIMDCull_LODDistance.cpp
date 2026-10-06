#include "../../StdAfx.h"
#include "ISIMDFrustumCuller.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "UserInterface/Core/EventBus.h"

#include <cmath>
#include <vector>
#include <expected>
#include <string_view>

namespace UserInterface::ECS::SIMD {

/**
 * @brief Event triggered when LOD culling batch is completed.
 * Enables strict GUI decoupling.
 */
struct LODCullingCompletedEvent : public Core::IEvent {
    size_t processedCount;
    size_t visibleCount;

    LODCullingCompletedEvent(size_t processed, size_t visible)
        : processedCount(processed), visibleCount(visible) {}
};

/**
 * @brief Evaluates Level of Detail (LOD) and culling based on squared distance from camera.
 * Implements ISIMDFrustumCuller contract.
 */
class SIMDCullLODDistance : public ISIMDFrustumCuller {
public:
    static constexpr float LOD_HIGH_DIST_SQ = 1500.0f * 1500.0f;
    static constexpr float LOD_MED_DIST_SQ  = 4000.0f * 4000.0f;
    static constexpr float LOD_LOW_DIST_SQ  = 12000.0f * 12000.0f;

    SIMDCullLODDistance() {
        EterBase::ModernLogger::Info("SIMDCullLODDistance initialized.");
    }

    ~SIMDCullLODDistance() override {
        Clear();
    }

    void SetFrustum(const FrustumPlanes& planes) override {
        m_planes = planes;
    }

    void BatchCullAABB(
        const float* minX, const float* minY, const float* minZ,
        const float* maxX, const float* maxY, const float* maxZ,
        uint8_t* outVisibleMask, size_t count) override 
    {
        if (auto res = ValidatePointers(minX, minY, minZ, maxX, maxY, maxZ, outVisibleMask); !res) {
            EterBase::ModernLogger::Error("BatchCullAABB failed: {}", res.error());
            return;
        }

        size_t visibleCount = 0;
        
        // Auto-vectorizable loop to determine LOD level based on distance
        for (size_t i = 0; i < count; ++i) {
            float cx = (minX[i] + maxX[i]) * 0.5f;
            float cy = (minY[i] + maxY[i]) * 0.5f;
            float cz = (minZ[i] + maxZ[i]) * 0.5f;

            float distSq = (cx * cx) + (cy * cy) + (cz * cz);

            if (distSq <= LOD_HIGH_DIST_SQ) {
                outVisibleMask[i] = 3; // High LOD
                visibleCount++;
            } else if (distSq <= LOD_MED_DIST_SQ) {
                outVisibleMask[i] = 2; // Medium LOD
                visibleCount++;
            } else if (distSq <= LOD_LOW_DIST_SQ) {
                outVisibleMask[i] = 1; // Low LOD
                visibleCount++;
            } else {
                outVisibleMask[i] = 0; // Culled / Too far
            }
        }

        Core::EventBus::GetInstance().Publish(LODCullingCompletedEvent{count, visibleCount});
    }

    size_t PackVisibleIndices(
        const uint8_t* visibleMask, size_t count, uint32_t* outIndices) override 
    {
        if (!visibleMask || !outIndices) {
            EterBase::ModernLogger::Error("PackVisibleIndices failed: Invalid pointers");
            return 0;
        }

        size_t outCount = 0;
        for (size_t i = 0; i < count; ++i) {
            if (visibleMask[i] > 0) {
                outIndices[outCount++] = static_cast<uint32_t>(i);
            }
        }
        return outCount;
    }

    void Clear() override {
        EterBase::ModernLogger::Debug("SIMDCullLODDistance cleared.");
    }

private:
    FrustumPlanes m_planes{};

    std::expected<void, std::string_view> ValidatePointers(
        const float* p1, const float* p2, const float* p3,
        const float* p4, const float* p5, const float* p6,
        const uint8_t* p7) const
    {
        if (!p1 || !p2 || !p3 || !p4 || !p5 || !p6 || !p7) {
            return std::unexpected("One or more array pointers are null");
        }
        return {};
    }
};

} // namespace UserInterface::ECS::SIMD
