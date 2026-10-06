#pragma once

#include <cstdint>
#include "../EventBus.h"
#include "../../EterBase/StrongTypes.h"

namespace UserInterface::Core::Events {

/**
 * @brief Event triggered when a quickslot is swapped with another.
 */
struct QuickslotSwapEvent : public IEvent {
    EterBase::ItemSlot pos;
    EterBase::ItemSlot changePos;

    QuickslotSwapEvent(EterBase::ItemSlot pos, EterBase::ItemSlot changePos)
        : pos(pos), changePos(changePos) {}
};

} // namespace UserInterface::Core::Events
