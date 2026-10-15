#pragma once

#include <cstdint>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::TextTail
{
    class ITargetHpBarService
    {
    public:
        virtual ~ITargetHpBarService() = default;

        virtual void ShowTargetBar(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) = 0;
        virtual void UpdateTargetHp(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) = 0;
        virtual void HideTargetBar() = 0;
        virtual void RenderTargetBar() = 0;
        virtual void RenderMiniMobBars() = 0;
        virtual void Clear() = 0;
    };
}
