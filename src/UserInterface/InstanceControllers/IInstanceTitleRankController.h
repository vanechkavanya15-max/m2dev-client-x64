#pragma once

#include <cstdint>
#include <string_view>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::InstanceControllers
{
    class IInstanceTitleRankController
    {
    public:
        virtual ~IInstanceTitleRankController() = default;

        virtual void SetAlignment(int32_t alignment) = 0;
        virtual int32_t GetAlignment() const = 0;
        virtual void SetPKMode(uint8_t pkMode) = 0;
        virtual uint8_t GetPKMode() const = 0;
        virtual void SetGuild(uint32_t guildId, std::string_view guildName) = 0;
        virtual uint32_t GetGuildId() const = 0;
        virtual void SetEmpire(uint8_t empire) = 0;
        virtual uint8_t GetEmpire() const = 0;
        virtual void SetCustomTitle(std::string_view title, uint32_t color) = 0;
        virtual void Clear() = 0;
    };
}
