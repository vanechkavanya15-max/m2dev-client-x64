#pragma once
#include "ITextTailService.h"
namespace UserInterface::TextTail
{
    class TextTailService : public ITextTailService
    {
    public:
        EterBase::PacketResult<void> RegisterActorTail(const TextTailCreateData& data) override;
        EterBase::PacketResult<void> RegisterItemTail(uint32_t virtualId, std::string_view name) override { return {}; }
        void RemoveTail(uint32_t virtualId) override {}
        void UpdateScreenPositions(float viewMatrix[16], float projMatrix[16]) override {}
        void RenderBatch() override {}
        void ClearAll() override {}
    };
}
