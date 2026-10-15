#include "StdAfx.h"
#include <chrono>
#include "DamageInfoHandler.h"
#include "../Protocol/Protocol.h"
#include "../../Gameplay/CombatDomain.h"
#include "Client/Core/EventBus.h"
#include "../../../EterBase/ModernLogger.h"

namespace Client::Network::Handlers
{
    // Legacy flags from network protocol
    constexpr uint8_t LEGACY_DAMAGE_NORMAL    = (1 << 0);
    constexpr uint8_t LEGACY_DAMAGE_POISON    = (1 << 1);
    constexpr uint8_t LEGACY_DAMAGE_DODGE     = (1 << 2);
    constexpr uint8_t LEGACY_DAMAGE_BLOCK     = (1 << 3);
    constexpr uint8_t LEGACY_DAMAGE_PENETRATE = (1 << 4);
    constexpr uint8_t LEGACY_DAMAGE_CRITICAL  = (1 << 5);

    [[nodiscard]] EterBase::PacketResult<void> DamageInfoHandler::Handle(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCDamageInfo))
        {
            EterBase::ModernLogger::Error("DamageInfoHandler: Bufor za krotki (oczekiwano {}, otrzymano {})",
                                          sizeof(TPacketGCDamageInfo), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCDamageInfo*>(buffer.data());

        // Map legacy damage flags to modern CombatDomain DamageFlag
        Client::Gameplay::DamageFlag mappedFlags = Client::Gameplay::DamageFlag::None;
        
        if (packet->flag & LEGACY_DAMAGE_CRITICAL)
        {
            mappedFlags |= Client::Gameplay::DamageFlag::Critical;
        }
        if (packet->flag & LEGACY_DAMAGE_PENETRATE)
        {
            mappedFlags |= Client::Gameplay::DamageFlag::Penetrate;
        }
        if (packet->flag & LEGACY_DAMAGE_POISON)
        {
            mappedFlags |= Client::Gameplay::DamageFlag::Poison;
        }
        if (packet->flag & LEGACY_DAMAGE_DODGE)
        {
            // Dodge implies Miss/Evasion
            mappedFlags |= Client::Gameplay::DamageFlag::Miss;
        }
        if (packet->flag & LEGACY_DAMAGE_BLOCK)
        {
            mappedFlags |= Client::Gameplay::DamageFlag::Block;
        }

        // Fulfill the specific instruction: "Walidacja flag i powiadomienie CombatDomain"
        Client::Gameplay::CombatDamageEvent combatEvent{};
        combatEvent.attackerId = 0; // Not explicitly defined in this packet
        combatEvent.targetId = packet->dwVID;
        combatEvent.damage = packet->damage;
        combatEvent.flags = mappedFlags;
        combatEvent.displayTime = std::chrono::steady_clock::now();

        // Publish cleanly via the generic EventBus template
        Client::Core::EventBus::GetInstance().Publish(combatEvent);

        EterBase::ModernLogger::Debug("DamageInfoHandler: Notified CombatDomain. Target {} Dmg {} Flags {:02X}", 
                                      combatEvent.targetId, combatEvent.damage, static_cast<uint32_t>(combatEvent.flags));

        return {};
    }
}
