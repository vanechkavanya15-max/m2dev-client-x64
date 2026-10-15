#pragma once
#include <span>
#include <cstdint>
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "Client/Core/EventBus.h"

namespace Client::Gameplay {
    // Niestandardowe zdarzenie dla wymogu: Wprowadzenie postaci w stan lotu i upadku po silnym uderzeniu
    struct CombatDomainKnockbackEvent : public Client::Core::IEvent {
        EterBase::EntityId targetId;
        EterBase::EntityId attackerId;
        constexpr explicit CombatDomainKnockbackEvent(EterBase::EntityId target, EterBase::EntityId attacker) noexcept
            : targetId(target), attackerId(attacker) {}
    };
}

namespace Client::Network::Handlers {
    EterBase::PacketResult<void> HandleKnockbackPacket(std::span<const uint8_t> payload) noexcept;
}
