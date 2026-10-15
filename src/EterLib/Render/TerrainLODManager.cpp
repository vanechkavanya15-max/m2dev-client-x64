#include "TerrainLODManager.h"
#include <algorithm>

namespace EterLib::Render
{
    void TerrainLODManager::SetLODDistances(std::span<const float, 4> distances) noexcept
    {
        std::copy_n(distances.data(), 4, m_lodDistances.data());
    }

    uint32_t TerrainLODManager::CalculateLOD(float distance) const noexcept
    {
        for (uint32_t i = 0; i < 3; ++i)
        {
            if (distance < m_lodDistances[i])
            {
                return i;
            }
        }
        return 3;
    }

    uint32_t TerrainLODManager::GetStitchMask(
        uint32_t currentLOD,
        uint32_t northLOD,
        uint32_t southLOD,
        uint32_t eastLOD,
        uint32_t westLOD) const noexcept
    {
        uint32_t mask = 0;
        
        if (northLOD > currentLOD) mask |= 1;
        if (southLOD > currentLOD) mask |= 2;
        if (eastLOD > currentLOD) mask |= 4;
        if (westLOD > currentLOD) mask |= 8;
        
        return mask;
    }
}

