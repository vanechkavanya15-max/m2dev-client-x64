#include "../StdAfx.h"
#include "PlayerStatsService.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::Services
{
    void PlayerStatsService::Clear()
    {
        m_points.fill(0);
        m_view = PlayerPointsView{};
        EterBase::ModernLogger::Info("PlayerStatsService::Clear stats reset.");
    }
}
