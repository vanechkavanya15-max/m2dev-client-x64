#pragma once

#include "src/UserInterface/Core/EventBus.h"
#include <cstdint>

namespace UserInterface::Core::Events {

/**
 * @brief Zdarzenie emitowane po udanej inicjalizacji tabeli walki (CombatComponentTable).
 * Slouzy do powiadomienia GUI, ze podsystem walki (HP) jest gotowy.
 */
struct CombatTableInitializedEvent : public IEvent {
    size_t reservedCapacity;

    explicit CombatTableInitializedEvent(size_t capacity) : reservedCapacity(capacity) {}
};

} // namespace UserInterface::Core::Events
