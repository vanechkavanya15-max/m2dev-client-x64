#ifndef TEST_MODE_DISABLE_STDAFX
#include "../StdAfx.h"
#endif

#include "ActorLODSelector.h"

namespace EterLib::Render
{
    void ActorLODSelector::SetLODThresholds(float lod1Dist, float lod2Dist) noexcept
    {
        m_lod1DistSq = lod1Dist * lod1Dist;
        m_lod2DistSq = lod2Dist * lod2Dist;
    }

    uint32_t ActorLODSelector::SelectLOD(float distanceSq) const noexcept
    {
        if (distanceSq >= m_lod2DistSq)
        {
            return 2;
        }
        else if (distanceSq >= m_lod1DistSq)
        {
            return 1;
        }
        
        return 0;
    }
} // namespace EterLib::Render

