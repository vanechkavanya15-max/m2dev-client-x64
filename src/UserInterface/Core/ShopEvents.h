#pragma once

#include "EventBus.h"
#include <string>
#include <cstdint>

namespace UserInterface::Core {

#pragma pack(push, 1)

struct PrivateShopDisappearEvent : public IEvent {
    uint32_t vid;
};

struct PrivateShopAppearEvent : public IEvent {
    uint32_t vid;
    std::string sign;
};

#pragma pack(pop)

}
