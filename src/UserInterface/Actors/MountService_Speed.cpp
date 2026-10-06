#include "../StdAfx.h"
#include "IMountHorseService.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include <unordered_map>
#include <mutex>

namespace UserInterface::Actors
{
    struct MountSpeedBonusCalculatedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId riderId;
        uint32_t speedBonus;
        MountSpeedBonusCalculatedEvent(EterBase::EntityId id, uint32_t bonus) : riderId(id), speedBonus(bonus) {}
    };

    class MountService_Speed
    {
    public:
        static uint32_t CalculateSpeedBonus(EterBase::EntityId riderId, uint32_t mountVnum)
        {
            uint32_t bonus = 30; // standard mount speed bonus
            if (mountVnum >= 20110 && mountVnum <= 20120)
                bonus = 50;

            UserInterface::Core::EventBus::GetInstance().Publish(MountSpeedBonusCalculatedEvent(riderId, bonus));
            EterBase::ModernLogger::Debug("MountService_Speed: Rider {} speed bonus {} for mount {}", riderId.get(), bonus, mountVnum);
            return bonus;
        }
    };
}
