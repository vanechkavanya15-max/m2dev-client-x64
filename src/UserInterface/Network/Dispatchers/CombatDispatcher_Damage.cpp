#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

#include <cstdint>
#include <span>

namespace UserInterface::Network::Dispatchers
{
    struct DamageInfoEvent : public UserInterface::Core::IEvent
    {
        uint32_t targetVid;
        uint8_t flag;
        int32_t damage;

        DamageInfoEvent(uint32_t vid, uint8_t f, int32_t dmg) : targetVid(vid), flag(f), damage(dmg) {}
    };

    class CombatDispatcher_Damage
    {
    public:
        static EterBase::PacketResult<void> Process(std::span<const uint8_t> buffer)
        {
            if (buffer.size() < sizeof(TPacketGCDamageInfo))
            {
                EterBase::ModernLogger::Error("CombatDispatcher_Damage: Za krotki bufor");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            const auto* packet = reinterpret_cast<const TPacketGCDamageInfo*>(buffer.data());
            UserInterface::Core::EventBus::GetInstance().Publish(DamageInfoEvent(packet->dwVID, packet->flag, packet->damage));
            EterBase::ModernLogger::Debug("CombatDispatcher_Damage: VID {} dmg {} flag {}", packet->dwVID, packet->damage, packet->flag);

            return {};
        }
    };
}
