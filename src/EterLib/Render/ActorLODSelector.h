#pragma once

#include <cstdint>

namespace EterLib::Render
{
    class ActorLODSelector
    {
    public:
        void SetLODThresholds(float lod1Dist, float lod2Dist) noexcept;
        [[nodiscard]] uint32_t SelectLOD(float distanceSq) const noexcept;

    private:
        float m_lod1DistSq = 0.0f;
        float m_lod2DistSq = 0.0f;
    };
} // namespace EterLib::Render

