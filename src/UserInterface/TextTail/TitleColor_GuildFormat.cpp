#include "../StdAfx.h"
#include "TitleColor_GuildFormat.h"
#include "GuildNameFormattedEvent.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include <format>
#include <string>
#include <string_view>

namespace UserInterface::TextTail
{
    std::string TitleColor_GuildFormat::FormatGuildName(std::string_view guildName) const
    {
        if (guildName.empty())
        {
            return "";
        }

        std::string result = std::format("[{}]", guildName);
        Core::EventBus::GetInstance().Publish(GuildNameFormattedEvent{guildName, result});
        return result;
    }
}
