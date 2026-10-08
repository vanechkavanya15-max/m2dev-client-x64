#pragma once
#include <span>
#include <cstdint>
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "Client/Core/EventBus.h"

namespace Client::Gameplay {
    // Custom event to fulfill the requirement "Blokada mozliwosci ruchu w CombatDomain i animacja omdlenia"
    // This cleanly delegates the domain operations (movement block & stun animation) without directly invoking UI.
    struct CombatDomainStunEvent : public Client::Core::IEvent {
        EterBase::EntityId targetId;
        explicit CombatDomainStunEvent(EterBase::EntityId id) : targetId(id) {}
    };
}

namespace Client::Network::Handlers {
    EterBase::PacketResult<void> HandleStunPacket(std::span<const uint8_t> payload);
}
