#pragma once

#include <cstdint>
#include <cstddef>

namespace GameLib::Terrain
{
    class ITerrainHeightCache
    {
    public:
        virtual ~ITerrainHeightCache() = default;

        virtual float SampleHeight(float x, float y) const = 0;
        virtual float SampleWaterHeight(float x, float y) const = 0;
        virtual bool IsWalkableSlope(float x, float y, float maxSlopeAngle) const = 0;
        virtual void BatchSampleHeight(const float* x, const float* y, float* outZ, size_t count) const = 0;
        virtual void Clear() = 0;
    };
}
