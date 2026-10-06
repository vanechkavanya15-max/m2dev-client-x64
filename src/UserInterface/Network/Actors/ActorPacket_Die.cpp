#include "../../StdAfx.h"
#include "IActorNetworkDispatcher.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include "../../Core/CombatEvents.h"

#include <cstdint>
#include <span>

namespace UserInterface::Network::Actors
{
#pragma pack(push, 1)
    struct PacketDead
    {
        uint16_t header;
        uint16_t length;
        uint32_t vid;
    };
#pragma pack(pop)

    EterBase::PacketResult<void> HandleCharacterDie(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(PacketDead))
        {
            EterBase::ModernLogger::Error("ActorPacket_Die: Buffer underflow. Expected >= {} bytes, got {}",
                sizeof(PacketDead), payload.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketDead*>(payload.data());
        EterBase::EntityId entityId{packet->vid};

        EterBase::ModernLogger::Info("ActorPacket_Die: Character {} died.", packet->vid);

        auto& eventBus = UserInterface::Core::EventBus::GetInstance();
        
        // Emisja zdarzenia ogolnego o smierci aktora (np. dla UI)
        eventBus.Publish(UserInterface::Core::ActorDeadEvent{packet->vid});

        // Emisja zdarzenia domeny walki
        auto targetDiedEventResult = UserInterface::Core::CombatEvents::TargetDied::Create(entityId, std::nullopt);
        if (targetDiedEventResult.has_value())
        {
            eventBus.Publish(targetDiedEventResult.value());
        }

        return {};
    }
}
