#pragma once

#include "ITitleNameColorizer.h"
#include <string>
#include <string_view>

namespace UserInterface::TextTail
{
    class TitleColor_GuildFormat : public virtual ITitleNameColorizer
    {
    public:
        std::string FormatGuildName(std::string_view guildName) const override;
    };
}
