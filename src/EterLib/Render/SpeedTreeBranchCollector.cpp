#include "SpeedTreeBranchCollector.h"
#include "SortKeyBuilder.h"
#include "DrawIndexedCommand.h"

namespace EterLib::Render
{
    void SpeedTreeBranchCollector::SubmitBranch(LPDIRECT3DVERTEXBUFFER9 vb, 
                                                LPDIRECT3DINDEXBUFFER9 ib, 
                                                uint32_t numPrims, 
                                                const D3DMATRIX& world, 
                                                RenderQueue& queue, 
                                                LinearFrameAllocator& alloc)
    {
        if (!vb || !ib || numPrims == 0)
            return;

        // Opaque pass key for tree branches
        SortKeyBuilder builder;
        builder.WithPass(static_cast<uint8_t>(Pass::Opaque));
        RenderSortKey sortKey = builder.Build();

        auto* cmd = alloc.AllocateObject<DrawIndexedCommand>();
        if (!cmd)
            return;

        cmd->primitiveType = D3DPT_TRIANGLELIST;
        cmd->baseVertexIndex = 0;
        cmd->minVertexIndex = 0;
        cmd->numVertices = numPrims * 3;
        cmd->startIndex = 0;
        cmd->primitiveCount = numPrims;
        cmd->vertexBuffer = vb;
        cmd->indexBuffer = ib;
        cmd->stride = 0; // Using 0 for stride as vertex format is handled elsewhere

        queue.Submit(sortKey, cmd, CommandType::Draw);
    }
}

