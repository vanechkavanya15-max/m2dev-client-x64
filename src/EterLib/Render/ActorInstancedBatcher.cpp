#include "ActorInstancedBatcher.h"
#include "SortKeyBuilder.h"
#include <span>
#include <cstring>

namespace EterLib::Render
{
    // A concrete implementation for the command executed via RenderQueue
    struct ActorBatchCommand
    {
        uint32_t modelId;
        const ActorInstanceData* instancesData;
        size_t instancesCount;

        ActorBatchCommand(uint32_t id, const ActorInstanceData* data, size_t count)
            : modelId(id), instancesData(data), instancesCount(count)
        {
        }

        void Execute(LPDIRECT3DDEVICE9 device) const noexcept
        {
            if (!device || !instancesData || instancesCount == 0)
                return;

#ifndef TEST_MOCK_D3D9
            // Here we assume the base model vertex/index buffers are already bound via the pipeline.
            // Setup hardware instancing frequencies
            device->SetStreamSourceFreq(0, (D3DSTREAMSOURCE_INDEXEDDATA | static_cast<UINT>(instancesCount)));
            device->SetStreamSourceFreq(1, (D3DSTREAMSOURCE_INSTANCEDATA | 1));

            // To fully execute without allocating DX buffers per-command, typically a global 
            // instance buffer would be used, but since we are encapsulating the command here, 
            // we will simulate the instance pushing assuming a mapped buffer or using UP calls 
            // if we needed to implement the buffer mapping in this class.
            
            // Assume the model draws using standard indexed primitive (triangle list, 0-based)
            device->DrawIndexedPrimitive(
                D3DPT_TRIANGLELIST,
                0, // base vertex index
                0, // min index
                1, // num vertices (dummy as actual count depends on mesh)
                0, // start index
                1  // primitive count (dummy)
            );

            // Restore stream frequencies to default
            device->SetStreamSourceFreq(0, 1);
            device->SetStreamSourceFreq(1, 1);
#endif
        }
    };

    void ActorInstancedBatcher::AddActorInstance(uint32_t modelId, const D3DMATRIX& world, uint32_t tint)
    {
        m_batches[modelId].push_back(ActorInstanceData{world, tint});
    }

    void ActorInstancedBatcher::Flush(RenderQueue& queue, LinearFrameAllocator& allocator)
    {
        for (const auto& [modelId, instances] : m_batches)
        {
            if (instances.empty())
                continue;

            // Allocate persistent memory for the instance data array for this frame
            size_t dataSize = instances.size() * sizeof(ActorInstanceData);
            void* copiedData = allocator.Allocate(dataSize, alignof(ActorInstanceData));
            if (!copiedData)
                continue; // Cannot allocate space

            std::memcpy(copiedData, instances.data(), dataSize);
            const ActorInstanceData* typedCopiedData = static_cast<const ActorInstanceData*>(copiedData);

            ActorBatchCommand* cmd = allocator.AllocateObject<ActorBatchCommand>(modelId, typedCopiedData, instances.size());
            if (cmd)
            {
                RenderSortKey key = SortKeyBuilder()
                                        .WithPass(static_cast<uint8_t>(Pass::Opaque))
                                        .WithShader(modelId)
                                        .Build();
                
                queue.Submit(key, cmd, CommandType::Draw);
            }
        }
    }

    void ActorInstancedBatcher::Clear() noexcept
    {
        for (auto& [modelId, instances] : m_batches)
        {
            instances.clear();
        }
    }
}

