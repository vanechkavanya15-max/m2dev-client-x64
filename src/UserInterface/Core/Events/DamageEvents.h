#pragma once

#include <cstdint>
#include "../EventBus.h"
#include "EterBase/StrongTypes.h"

namespace UserInterface::Core::Events {

/**
 * @brief Event triggered when damage information is received from the server.
 */
struct DamageInfoEvent : public IEvent {
    EterBase::EntityId targetId;
    int32_t damage;
    uint8_t flag;

    DamageInfoEvent(EterBase::EntityId targetId, int32_t damage, uint8_t flag)
        : targetId(targetId), damage(damage), flag(flag) {}
};

} // namespace UserInterface::Core::Events
