#include "../../StdAfx.h"
#include "IActorNetworkDispatcher.h"
#include "../../Packets/Packet_DamageInfo.h"
#include "../../Core/Events/DamageEvents.h"
#include "../../Core/EventBus.h"
#include "EterBase/LogModern.h"

namespace UserInterface::Network
{
    EterBase::PacketResult<void> IActorNetworkDispatcher::HandleDamageInfo(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(TPacketGCDamageInfo)) {
            EterBase::ModernLogger::Error("ActorPacket_DamageInfo: Buffer underflow. Expected {}, got {}", 
                                          sizeof(TPacketGCDamageInfo), payload.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCDamageInfo*>(payload.data());
        
        EterBase::EntityId targetId{ packet->targetId };
        
        EterBase::ModernLogger::Debug("ActorPacket_DamageInfo: Target {}, Damage {}, Flag {}", 
                                      targetId.value(), packet->damage, packet->flag);

        Core::EventBus::GetInstance().Publish(
            Core::Events::DamageInfoEvent{ targetId, packet->damage, packet->flag }
        );

        return {};
    }
}
