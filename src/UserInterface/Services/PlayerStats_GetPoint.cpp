#include "../StdAfx.h"
#include "PlayerStatsService.h"
#include "Client/Bridge/StranglerFacade.h"

namespace UserInterface::Services
{
    int64_t PlayerStatsService::GetPoint(uint32_t type) const
    {
        return Client::Bridge::StranglerFacade::Instance().GetWorldContext().GetPoint(type);
    }

    const PlayerPointsView& PlayerStatsService::GetPoints() const
    {
        return m_view;
    }
}
