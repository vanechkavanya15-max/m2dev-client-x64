#include "GrannyRenderBridge.h"
#include "RenderQueue.h"
#include "LinearFrameAllocator.h"


namespace EterLib::Render
{
    void GrannyRenderBridge::SubmitModel(uint32_t meshId, const D3DMATRIX& world, RenderQueue& /*queue*/, LinearFrameAllocator& /*alloc*/)
    {
        m_batcher.AddInstance(meshId, world);
    }

    void GrannyRenderBridge::FlushPending(RenderQueue& queue, LinearFrameAllocator& alloc)
    {
        const auto& batches = m_batcher.GetBatches();

        for (const auto& [meshId, instances] : batches)
        {
            if (instances.empty())
                continue;

            size_t instanceCount = instances.size();
            size_t memSize = sizeof(D3DMATRIX) * instanceCount;
            
            D3DMATRIX* instancesCopy = static_cast<D3DMATRIX*>(alloc.Allocate(memSize, alignof(D3DMATRIX)));
            if (!instancesCopy)
                continue; // OOM in frame allocator
                
            for (size_t i = 0; i < instanceCount; ++i)
            {
                instancesCopy[i] = instances[i];
            }

            ActorDrawCommand* cmd = alloc.AllocateObject<ActorDrawCommand>();
            if (!cmd)
                continue;

            cmd->meshId = meshId;
            cmd->instances = instancesCopy;
            cmd->instanceCount = instanceCount;

            RenderSortKey key;
            key.value = meshId; // simplified sort key

            queue.Submit(key, cmd, CommandType::Draw);
        }

        m_batcher.Clear();
    }
} // namespace EterLib::Render

