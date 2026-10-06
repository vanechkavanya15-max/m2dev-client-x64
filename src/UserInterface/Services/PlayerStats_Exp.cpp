#include "../StdAfx.h"
#include "PlayerStatsService.h"
#include "../Core/EventBus.h"

namespace UserInterface::Services
{
    struct PlayerExpPercentUpdatedEvent : public UserInterface::Core::IEvent
    {
        float percent;
        explicit PlayerExpPercentUpdatedEvent(float p) : percent(p) {}
    };

    float CalculateExpPercent(uint64_t currentExp, uint64_t nextExp)
    {
        if (nextExp == 0)
            return 0.0f;
        float pct = (static_cast<float>(currentExp) / static_cast<float>(nextExp)) * 100.0f;
        UserInterface::Core::EventBus::GetInstance().Publish(PlayerExpPercentUpdatedEvent(pct));
        return pct;
    }
}
