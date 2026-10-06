#include "../StdAfx.h"
#include "PlayerStatsService.h"

namespace UserInterface::Services
{
    int64_t PlayerStatsService::GetPoint(uint32_t type) const
    {
        if (type >= m_points.size())
            return 0;
        return m_points[type];
    }

    const PlayerPointsView& PlayerStatsService::GetPoints() const
    {
        return m_view;
    }
}
