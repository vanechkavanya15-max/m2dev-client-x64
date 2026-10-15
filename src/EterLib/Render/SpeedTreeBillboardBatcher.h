#pragma once

#include <vector>
#include <cstdint>
#include <d3d9.h>

namespace EterLib::Render
{
    class RenderQueue;
    class LinearFrameAllocator;

    class SpeedTreeBillboardBatcher
    {
    public:
        SpeedTreeBillboardBatcher() = default;
        ~SpeedTreeBillboardBatcher() = default;

        SpeedTreeBillboardBatcher(const SpeedTreeBillboardBatcher&) = delete;
        SpeedTreeBillboardBatcher& operator=(const SpeedTreeBillboardBatcher&) = delete;

        void AddTreeBillboard(const D3DVECTOR& pos, float width, float height, uint32_t textureId);
        void Submit(RenderQueue& queue, LinearFrameAllocator& alloc);
        void Clear() noexcept;

    private:
        struct BillboardInstance
        {
            D3DVECTOR pos;
            float width;
            float height;
            uint32_t textureId;

            bool operator<(const BillboardInstance& other) const
            {
                return textureId < other.textureId;
            }
        };

        std::vector<BillboardInstance> m_billboards;
    };
} // namespace EterLib::Render

