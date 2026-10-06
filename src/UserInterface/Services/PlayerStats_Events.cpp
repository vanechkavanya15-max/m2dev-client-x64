#include "../StdAfx.h"
#include "PlayerStatsService.h"
#include "../Core/EventBus.h"

namespace UserInterface::Services
{
    struct PlayerHpChangedEvent : public UserInterface::Core::IEvent
    {
        uint32_t currentHp;
        uint32_t maxHp;
        PlayerHpChangedEvent(uint32_t c, uint32_t m) : currentHp(c), maxHp(m) {}
    };

    struct PlayerSpChangedEvent : public UserInterface::Core::IEvent
    {
        uint32_t currentSp;
        uint32_t maxSp;
        PlayerSpChangedEvent(uint32_t c, uint32_t m) : currentSp(c), maxSp(m) {}
    };

    void PublishStatsEvents(const PlayerPointsView& points)
    {
        UserInterface::Core::EventBus::GetInstance().Publish(PlayerHpChangedEvent(points.hp, points.maxHp));
        UserInterface::Core::EventBus::GetInstance().Publish(PlayerSpChangedEvent(points.sp, points.maxSp));
    }
}
