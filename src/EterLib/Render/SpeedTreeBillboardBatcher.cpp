#include "SpeedTreeBillboardBatcher.h"
#include "RenderQueue.h"
#include "LinearFrameAllocator.h"
#include "DrawUserPrimitiveCommand.h"
#include "SortKeyBuilder.h"
#include "../GrpBase.h"

#include <algorithm>

namespace EterLib::Render
{
    void SpeedTreeBillboardBatcher::AddTreeBillboard(const D3DVECTOR& pos, float width, float height, uint32_t textureId)
    {
        m_billboards.push_back({pos, width, height, textureId});
    }

    void SpeedTreeBillboardBatcher::Clear() noexcept
    {
        m_billboards.clear();
    }

    void SpeedTreeBillboardBatcher::Submit(RenderQueue& queue, LinearFrameAllocator& alloc)
    {
        if (m_billboards.empty())
            return;

        const D3DXMATRIX& viewMat = CGraphicBase::GetViewMatrix();
        D3DXVECTOR3 right(viewMat._11, viewMat._21, viewMat._31);
        D3DXVECTOR3 up(viewMat._12, viewMat._22, viewMat._32);

        std::sort(m_billboards.begin(), m_billboards.end());

        auto it = m_billboards.begin();
        while (it != m_billboards.end())
        {
            uint32_t currentTexture = it->textureId;
            auto rangeEnd = std::find_if(it, m_billboards.end(), [currentTexture](const BillboardInstance& b) {
                return b.textureId != currentTexture;
            });

            size_t count = std::distance(it, rangeEnd);
            size_t vertexCount = count * 6; // 2 triangles = 6 vertices per quad

            std::vector<TPDTVertex> vertices;
            vertices.reserve(vertexCount);

            for (auto curr = it; curr != rangeEnd; ++curr)
            {
                float halfWidth = curr->width * 0.5f;
                float halfHeight = curr->height * 0.5f;

                D3DXVECTOR3 rightScaled = right * halfWidth;
                D3DXVECTOR3 upScaled = up * halfHeight;

                D3DXVECTOR3 center(curr->pos.x, curr->pos.y, curr->pos.z);

                D3DXVECTOR3 v0 = center - rightScaled - upScaled; // Bottom-left
                D3DXVECTOR3 v1 = center + rightScaled - upScaled; // Bottom-right
                D3DXVECTOR3 v2 = center - rightScaled + upScaled; // Top-left
                D3DXVECTOR3 v3 = center + rightScaled + upScaled; // Top-right

                DWORD color = 0xFFFFFFFF; // White, fully opaque

                TPDTVertex tv0{ {v0.x, v0.y, v0.z}, color, {0.0f, 1.0f} };
                TPDTVertex tv1{ {v1.x, v1.y, v1.z}, color, {1.0f, 1.0f} };
                TPDTVertex tv2{ {v2.x, v2.y, v2.z}, color, {0.0f, 0.0f} };
                TPDTVertex tv3{ {v3.x, v3.y, v3.z}, color, {1.0f, 0.0f} };

                // Triangle 1: v0, v2, v1
                vertices.push_back(tv0);
                vertices.push_back(tv2);
                vertices.push_back(tv1);

                // Triangle 2: v1, v2, v3
                vertices.push_back(tv1);
                vertices.push_back(tv2);
                vertices.push_back(tv3);
            }

            auto* cmd = DrawUserPrimitiveCommand::Allocate(alloc, D3DPT_TRIANGLELIST, count * 2, vertices.data(), sizeof(TPDTVertex));
            if (cmd)
            {
                RenderSortKey key = SortKeyBuilder()
                                        .WithPass(static_cast<uint8_t>(Pass::AlphaTest))
                                        .WithTexture(currentTexture)
                                        .Build();
                
                queue.Submit(key, cmd, CommandType::Draw);
            }

            it = rangeEnd;
        }
    }
} // namespace EterLib::Render

