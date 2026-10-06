#pragma once

#include <cstdint>
#include <cstddef>

namespace UserInterface::ECS::SIMD
{
    struct FrustumPlanes
    {
        float planes[6][4]; // 6 planes: Ax + By + Cz + D = 0
    };

    class ISIMDFrustumCuller
    {
    public:
        virtual ~ISIMDFrustumCuller() = default;

        virtual void SetFrustum(const FrustumPlanes& planes) = 0;
        virtual void BatchCullAABB(
            const float* minX, const float* minY, const float* minZ,
            const float* maxX, const float* maxY, const float* maxZ,
            uint8_t* outVisibleMask, size_t count) = 0;

        virtual size_t PackVisibleIndices(
            const uint8_t* visibleMask, size_t count, uint32_t* outIndices) = 0;

        virtual void Clear() = 0;
    };
}
