#include "../../StdAfx.h"
#include "IActorNetworkDispatcher.h"
#include "../../Packet.h"
#include "../../Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"

namespace UserInterface::Network::Actors
{
    struct CharacterDeleteEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
    };

    class ActorNetworkDispatcher : public IActorNetworkDispatcher
    {
    public:
        EterBase::PacketResult<void> HandleCharacterDelete(std::span<const uint8_t> payload) override
        {
            if (payload.size() < sizeof(TPacketGCCharacterDelete))
            {
                EterBase::ModernLogger::Error("Buffer underflow in HandleCharacterDelete");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            const auto* packet = reinterpret_cast<const TPacketGCCharacterDelete*>(payload.data());
            EterBase::EntityId entityId(packet->dwVID);

            EterBase::ModernLogger::Info("ActorNetworkDispatcher::HandleCharacterDelete - deleting entity: {}", packet->dwVID);

            CharacterDeleteEvent event;
            event.entityId = entityId;
            Core::EventBus::GetInstance().Publish(event);

            return {};
        }

        EterBase::PacketResult<void> HandleCharacterAdd(std::span<const uint8_t> payload) override
        {
            EterBase::ModernLogger::Error("HandleCharacterAdd not implemented in ActorPacket_CharDel");
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }

        EterBase::PacketResult<void> HandleCharacterUpdate(std::span<const uint8_t> payload) override
        {
            EterBase::ModernLogger::Error("HandleCharacterUpdate not implemented in ActorPacket_CharDel");
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }

        EterBase::PacketResult<void> HandleCharacterMove(std::span<const uint8_t> payload) override
        {
            EterBase::ModernLogger::Error("HandleCharacterMove not implemented in ActorPacket_CharDel");
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }

        EterBase::PacketResult<void> HandleDamageInfo(std::span<const uint8_t> payload) override
        {
            EterBase::ModernLogger::Error("HandleDamageInfo not implemented in ActorPacket_CharDel");
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }

        EterBase::PacketResult<void> HandleCharacterDie(std::span<const uint8_t> payload) override
        {
            EterBase::ModernLogger::Error("HandleCharacterDie not implemented in ActorPacket_CharDel");
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }

        void Clear() override
        {
            EterBase::ModernLogger::Error("Clear not implemented in ActorPacket_CharDel");
        }
    };
} // namespace UserInterface::Network::Actors
