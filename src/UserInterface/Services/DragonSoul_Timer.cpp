#include "../StdAfx.h"
#include "ISpecialInventoryService.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include <unordered_map>

namespace UserInterface::Services
{
    struct DragonSoulTimerExpiredEvent : public Core::IEvent
    {
        uint8_t deck;
        EterBase::ItemSlot slot;

        DragonSoulTimerExpiredEvent(uint8_t d, EterBase::ItemSlot s) : deck(d), slot(s) {}
    };

    class DragonSoulTimerTracker
    {
    private:
        struct TimerEntry
        {
            uint8_t deck;
            EterBase::ItemSlot slot;
            float remainingTime;
        };

        std::unordered_map<uint32_t, TimerEntry> m_timers;

    public:
        void SetTimer(uint8_t deck, EterBase::ItemSlot slot, float duration)
        {
            uint32_t key = (static_cast<uint32_t>(deck) << 16) | slot.value();
            m_timers[key] = TimerEntry{deck, slot, duration};
            EterBase::ModernLogger::Debug("DragonSoulTimerTracker: Timer set deck {} slot {} dur {}", deck, slot.value(), duration);
        }

        void Update(float deltaTime)
        {
            for (auto it = m_timers.begin(); it != m_timers.end();)
            {
                it->second.remainingTime -= deltaTime;
                if (it->second.remainingTime <= 0.0f)
                {
                    Core::EventBus::GetInstance().Publish(DragonSoulTimerExpiredEvent(it->second.deck, it->second.slot));
                    EterBase::ModernLogger::Info("DragonSoulTimerTracker: Timer expired for deck {} slot {}",
                        it->second.deck, it->second.slot.value());
                    it = m_timers.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void Clear()
        {
            m_timers.clear();
        }
    };
}
