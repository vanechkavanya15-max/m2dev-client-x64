#include "../StdAfx.h"
#include "PlayerStatsService.h"
#include "../Core/EventBus.h"

namespace UserInterface::Services
{
    struct DerivedStatsRecalculatedEvent : public UserInterface::Core::IEvent
    {
        uint32_t attackPower;
        uint32_t defense;
        DerivedStatsRecalculatedEvent(uint32_t att, uint32_t def) : attackPower(att), defense(def) {}
    };

    void RecalculateDerivedStats(const IPlayerStatsService& service)
    {
        uint32_t att = static_cast<uint32_t>(service.GetPoint(1)); // POINT_ATT_SPEED
        uint32_t def = static_cast<uint32_t>(service.GetPoint(54)); // POINT_DEF_GRADE
        UserInterface::Core::EventBus::GetInstance().Publish(DerivedStatsRecalculatedEvent(att, def));
    }
}
