#pragma once

#include <cstdint>
#include <cstddef>

namespace GameLib::Terrain
{
    class ITerrainQuadtreeCuller
    {
    public:
        virtual ~ITerrainQuadtreeCuller() = default;

        virtual void BuildQuadtree(int32_t sectorX, int32_t sectorY) = 0;
        virtual size_t CullTerrainPatches(const float viewFrustum[6][4], uint32_t* outVisiblePatchIds) = 0;
        virtual uint8_t SelectLOD(float distance) const = 0;
        virtual void Clear() = 0;
    };
}
