#pragma once

#include <functional>
#include <cstdint>
#include <optional>

#include "src/UserInterface/Core/CombatEvents.h"
#include "src/UserInterface/Core/EventBus.h"

namespace Client::Bridge {

/**
 * @brief Adapter tlumaczacy zdarzenia walki z EventBus na wywolania UI/Pythona.
 * 
 * Zapewnia RAII dla subskrypcji zdarzen ActorDamaged i TargetDied.
 */
class PyCombatEventAdapter {
public:
    using OnDamageCallback = std::function<void(uint32_t attackerVid, uint32_t victimVid, uint32_t damage, bool isCritical)>;
    using OnDeadCallback = std::function<void(uint32_t victimVid, std::optional<uint32_t> killerVid)>;

    PyCombatEventAdapter() = default;

    ~PyCombatEventAdapter() {
        UnsubscribeAll();
    }

    PyCombatEventAdapter(const PyCombatEventAdapter&) = delete;
    PyCombatEventAdapter& operator=(const PyCombatEventAdapter&) = delete;

    /**
     * @brief Inicjuje subskrypcje na EventBus.
     */
    void Initialize(OnDamageCallback damageCb, OnDeadCallback deadCb) {
        UnsubscribeAll();

        m_onDamage = std::move(damageCb);
        m_onDead = std::move(deadCb);

        auto& eventBus = UserInterface::Core::EventBus::Instance();

        m_damageSubId = eventBus.Subscribe<UserInterface::Core::CombatEvents::ActorDamaged>(
            [this](const UserInterface::Core::CombatEvents::ActorDamaged& event) {
                HandleActorDamaged(event);
            }
        );

        m_deadSubId = eventBus.Subscribe<UserInterface::Core::CombatEvents::TargetDied>(
            [this](const UserInterface::Core::CombatEvents::TargetDied& event) {
                HandleTargetDied(event);
            }
        );
    }

private:
    void HandleActorDamaged(const UserInterface::Core::CombatEvents::ActorDamaged& event) const {
        if (m_onDamage) {
            m_onDamage(
                event.attackerId.value(),
                event.victimId.value(),
                event.damageAmount,
                event.isCritical
            );
        }
    }

    void HandleTargetDied(const UserInterface::Core::CombatEvents::TargetDied& event) const {
        if (m_onDead) {
            std::optional<uint32_t> killerId;
            if (event.killerId.has_value()) {
                killerId = event.killerId.value().value();
            }
            m_onDead(event.targetId.value(), killerId);
        }
    }

    void UnsubscribeAll() {
        auto& eventBus = UserInterface::Core::EventBus::Instance();
        if (m_damageSubId.has_value()) {
            eventBus.Unsubscribe<UserInterface::Core::CombatEvents::ActorDamaged>(m_damageSubId.value());
            m_damageSubId.reset();
        }
        if (m_deadSubId.has_value()) {
            eventBus.Unsubscribe<UserInterface::Core::CombatEvents::TargetDied>(m_deadSubId.value());
            m_deadSubId.reset();
        }
    }

    std::optional<uint32_t> m_damageSubId;
    std::optional<uint32_t> m_deadSubId;
    OnDamageCallback m_onDamage;
    OnDeadCallback m_onDead;
};

} // namespace Client::Bridge
