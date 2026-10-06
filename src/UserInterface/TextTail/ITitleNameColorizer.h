#pragma once

#include <cstdint>
#include <string_view>
#include <string>

namespace UserInterface::TextTail
{
    class ITitleNameColorizer
    {
    public:
        virtual ~ITitleNameColorizer() = default;

        virtual uint32_t GetAlignmentColor(int32_t alignment) const = 0;
        virtual uint32_t GetEmpireColor(uint8_t empire) const = 0;
        virtual uint32_t GetLevelColor(int32_t playerLevel, int32_t mobLevel) const = 0;
        virtual std::string FormatGuildName(std::string_view guildName) const = 0;
        virtual std::string FormatAlignmentTitle(int32_t alignment) const = 0;
        virtual void Clear() = 0;
    };
}
