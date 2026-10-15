#include "StdAfx.h"
#include "StunHandler.h"
#include "../Protocol/Protocol.h"
#include "EterBase/LogModern.h"

namespace Client::Network::Handlers {
    EterBase::PacketResult<void> HandleStunPacket(std::span<const uint8_t> payload) {
        if (payload.size() < sizeof(TPacketGCStun)) {
            EterBase::ModernLogger::Error("HandleStunPacket: Payload too small. Expected {}, got {}",
                                          sizeof(TPacketGCStun), payload.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCStun*>(payload.data());
        const EterBase::EntityId targetId{packet->vid};

        EterBase::ModernLogger::Info("HandleStunPacket: Received stun packet for EntityId={}", targetId.value());

        // Emit the domain-specific event to trigger the stun animation and block movement in CombatDomain
        Client::Core::EventBus::GetInstance().Publish(
            Client::Gameplay::CombatDomainStunEvent{targetId}
        );

        return {};
    }
}
