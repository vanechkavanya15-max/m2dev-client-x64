#pragma once

#include "../EventBus.h"
#include "../../EterBase/StrongTypes.h"
#include <cstdint>

namespace UserInterface::Core::Events {

struct AffectAddEvent : public IEvent {
    uint32_t type{0};
    uint8_t pointIdxApplyOn{0};
    int32_t applyValue{0};
    uint32_t flag{0};
    int32_t duration{0};

    AffectAddEvent() = default;
    AffectAddEvent(uint32_t t, uint8_t p, int32_t v, uint32_t f, int32_t d)
        : type(t), pointIdxApplyOn(p), applyValue(v), flag(f), duration(d) {}
};

} // namespace UserInterface::Core::Events
