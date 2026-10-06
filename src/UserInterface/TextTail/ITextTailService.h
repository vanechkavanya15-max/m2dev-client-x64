#pragma once

#include <cstdint>
#include <string_view>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::TextTail
{
    struct TextTailCreateData
    {
        EterBase::EntityId vid{0};
        std::string_view text;
        uint32_t color{0xFFFFFFFF};
        float offsetY{0.0f};
    };

    class ITextTailService
    {
    public:
        virtual ~ITextTailService() = default;

        virtual EterBase::PacketResult<void> RegisterActorTail(const TextTailCreateData& data) = 0;
        virtual EterBase::PacketResult<void> RegisterItemTail(uint32_t virtualId, std::string_view name) = 0;
        virtual void RemoveTail(uint32_t virtualId) = 0;
        virtual void UpdateScreenPositions(float viewMatrix[16], float projMatrix[16]) = 0;
        virtual void RenderBatch() = 0;
        virtual void ClearAll() = 0;
    };
}
