#pragma once

#include <string_view>
#include <string>
#include "../Core/EventBus.h"

namespace UserInterface::TextTail
{
    struct GuildNameFormattedEvent : public Core::IEvent
    {
        std::string_view originalName;
        std::string formattedName;

        GuildNameFormattedEvent(std::string_view orig, std::string_view formatted)
            : originalName(orig), formattedName(formatted) {}
    };
}
