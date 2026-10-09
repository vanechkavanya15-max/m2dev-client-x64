#include "StdAfx.h"
#include "CombatKnockbackPacketHandler.h"
#include "../Protocol/Protocol.h"
#include "../../World/ActorMotionMachine.h"
#include "EterBase/LogModern.h"

namespace Client::Network::Handlers {
    EterBase::PacketResult<void> HandleKnockbackPacket(std::span<const uint8_t> payload) noexcept {
        if (payload.size() < sizeof(TPacketGCMotion)) {
            EterBase::ModernLogger::Error("HandleKnockbackPacket: Zbyt maly rozmiar bufora. Oczekiwano {}, otrzymano {}",
                                          sizeof(TPacketGCMotion), payload.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCMotion*>(payload.data());
        
        if (packet->motion != static_cast<uint16_t>(World::MotionState::Knockback)) {
            EterBase::ModernLogger::Error("HandleKnockbackPacket: Nieprawidlowy typ ruchu. Oczekiwano Knockback (9), otrzymano {}",
                                          packet->motion);
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        const EterBase::EntityId targetId{packet->vid};
        const EterBase::EntityId attackerId{packet->victim_vid}; // victim_vid w TPacketGCMotion reprezentuje cel lub sprawce zdarzenia
        
        EterBase::ModernLogger::Info("HandleKnockbackPacket: Otrzymano pakiet odrzutu dla TargetId={} AttackerId={}", targetId.value(), attackerId.value());

        // Wyemituj zdarzenie domenowe do wyzwolenia logiki odrzutu w CombatDomain
        Client::Core::EventBus::GetInstance().Publish(
            Client::Gameplay::CombatDomainKnockbackEvent{targetId, attackerId}
        );

        return {};
    }
}
