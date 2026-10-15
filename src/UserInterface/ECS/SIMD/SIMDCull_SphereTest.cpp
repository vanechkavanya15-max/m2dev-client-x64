#include "../../StdAfx.h"
#include "ISIMDFrustumCuller.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

#include <cmath>
#include <memory>
#include <string_view>

namespace UserInterface::ECS::SIMD
{

/**
 * @brief Event emitted when SIMD sphere culling completes.
 * Disconnects the core logic from GUI/rendering systems.
 */
struct SIMDSphereCullCompletedEvent : public UserInterface::Core::IEvent
{
    size_t totalProcessed;
    size_t visibleCount;

    SIMDSphereCullCompletedEvent(size_t total, size_t visible)
        : totalProcessed(total), visibleCount(visible) {}
};

/**
 * @brief Implementation of ISIMDFrustumCuller performing a fast bounding sphere test.
 */
class SIMDFrustumCuller_SphereTest final : public ISIMDFrustumCuller
{
public:
    SIMDFrustumCuller_SphereTest()
    {
        EterBase::ModernLogger::Debug("SIMDFrustumCuller_SphereTest initialized.");
    }

    ~SIMDFrustumCuller_SphereTest() override
    {
        EterBase::ModernLogger::Debug("SIMDFrustumCuller_SphereTest destroyed.");
    }

    void SetFrustum(const FrustumPlanes& planes) override
    {
        m_planes = planes;
    }

    void BatchCullAABB(
        const float* minX, const float* minY, const float* minZ,
        const float* maxX, const float* maxY, const float* maxZ,
        uint8_t* outVisibleMask, size_t count) override
    {
        if (auto valid = ValidateInput(minX, minY, minZ, maxX, maxY, maxZ, outVisibleMask); !valid)
        {
            EterBase::ModernLogger::Error("BatchCullAABB failed: {}", valid.error());
            return;
        }

        size_t visibleCount = 0;

        for (size_t i = 0; i < count; ++i)
        {
            // Approximate AABB to a bounding sphere
            const float cx = (minX[i] + maxX[i]) * 0.5f;
            const float cy = (minY[i] + maxY[i]) * 0.5f;
            const float cz = (minZ[i] + maxZ[i]) * 0.5f;

            const float dx = maxX[i] - minX[i];
            const float dy = maxY[i] - minY[i];
            const float dz = maxZ[i] - minZ[i];

            // Radius is half of the diagonal length
            const float radius = std::sqrt(dx * dx + dy * dy + dz * dz) * 0.5f;

            bool isVisible = true;
            for (size_t p = 0; p < 6; ++p)
            {
                // Calculate distance from sphere center to frustum plane
                const float distance = m_planes.planes[p][0] * cx +
                                       m_planes.planes[p][1] * cy +
                                       m_planes.planes[p][2] * cz +
                                       m_planes.planes[p][3];

                // If the center is further outside the plane than the radius, it's culled
                if (distance < -radius)
                {
                    isVisible = false;
                    break;
                }
            }

            outVisibleMask[i] = isVisible ? 1 : 0;
            if (isVisible)
            {
                visibleCount++;
            }
        }

        // Emitting event to decouple from GUI / rendering updates
        UserInterface::Core::EventBus::GetInstance().Publish(SIMDSphereCullCompletedEvent{count, visibleCount});
    }

    size_t PackVisibleIndices(
        const uint8_t* visibleMask, size_t count, uint32_t* outIndices) override
    {
        if (!visibleMask || !outIndices)
        {
            EterBase::ModernLogger::Error("PackVisibleIndices failed: Invalid pointers.");
            return 0;
        }

        size_t visibleCount = 0;
        for (size_t i = 0; i < count; ++i)
        {
            if (visibleMask[i])
            {
                // Utilize EterBase::EntityId to ensure strong typing logic internally
                EterBase::EntityId entityId{static_cast<uint32_t>(i)};
                outIndices[visibleCount++] = entityId.get();
            }
        }

        return visibleCount;
    }

    void Clear() override
    {
        EterBase::ModernLogger::Debug("SIMDFrustumCuller_SphereTest state cleared.");
    }

private:
    FrustumPlanes m_planes{};

    [[nodiscard]] EterBase::Result<void> ValidateInput(
        const float* minX, const float* minY, const float* minZ,
        const float* maxX, const float* maxY, const float* maxZ,
        const uint8_t* outVisibleMask) const
    {
        if (!minX || !minY || !minZ || !maxX || !maxY || !maxZ || !outVisibleMask)
        {
            return std::unexpected("Nullptr encountered in bounding box arrays or output mask.");
        }
        return {};
    }
};

/**
 * @brief Factory function to create the sphere culler (Zero-conflict).
 */
[[nodiscard]] std::unique_ptr<ISIMDFrustumCuller> CreateSIMDSphereCuller()
{
    return std::make_unique<SIMDFrustumCuller_SphereTest>();
}

} // namespace UserInterface::ECS::SIMD
