#pragma once

#include <cstdint>
#include <string_view>
#include <string>
#include <optional>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::GroundDrop
{
    struct FormattedTooltipData
    {
        std::string title;
        std::string description;
        uint32_t titleColor{0xFFFFFFFF};
        int32_t sellPrice{0};
    };

    class IFastItemTooltipCache
    {
    public:
        virtual ~IFastItemTooltipCache() = default;

        virtual void CacheTooltip(EterBase::ItemVnum vnum, const FormattedTooltipData& data) = 0;
        virtual std::optional<FormattedTooltipData> GetTooltip(EterBase::ItemVnum vnum) const = 0;
        virtual void Invalidate(EterBase::ItemVnum vnum) = 0;
        virtual void ClearAll() = 0;
    };
}
