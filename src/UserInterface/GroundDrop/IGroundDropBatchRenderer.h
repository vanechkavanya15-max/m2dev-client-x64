#pragma once

#include <cstdint>
#include <string_view>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::GroundDrop
{
    struct GroundDropItemData
    {
        uint32_t virtualId{0};
        EterBase::ItemVnum itemVnum{0};
        uint32_t count{1};
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};
        uint32_t ownerVid{0};
        uint32_t expirationTime{0};
    };

    class IGroundDropBatchRenderer
    {
    public:
        virtual ~IGroundDropBatchRenderer() = default;

        virtual EterBase::PacketResult<void> AddDrop(const GroundDropItemData& item) = 0;
        virtual EterBase::PacketResult<void> RemoveDrop(uint32_t virtualId) = 0;
        virtual void UpdateDrops(float deltaTime) = 0;
        virtual void RenderInstancedDrops() = 0;
        virtual bool IsInRangeToPick(uint32_t virtualId, float playerX, float playerY, float maxRange) const = 0;
        virtual void ClearAll() = 0;
    };
}
