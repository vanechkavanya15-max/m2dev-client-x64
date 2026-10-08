#pragma once

#ifndef TEST_MOCK_D3D9
#include <d3d9.h>
#else
#include "d3d9.h"
#include "d3dx9math.h"
#endif

#include "src/EterLib/Render/RenderQueue.h"
#include "src/EterLib/Render/LinearFrameAllocator.h"
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace EterLib::Render
{
    struct ActorInstanceData
    {
        D3DMATRIX world;
        uint32_t tint;
    };

    class ActorInstancedBatcher
    {
    public:
        ActorInstancedBatcher() = default;
        ~ActorInstancedBatcher() = default;

        // Non-copyable
        ActorInstancedBatcher(const ActorInstancedBatcher&) = delete;
        ActorInstancedBatcher& operator=(const ActorInstancedBatcher&) = delete;

        void AddActorInstance(uint32_t modelId, const D3DMATRIX& world, uint32_t tint);
        void Flush(RenderQueue& queue, LinearFrameAllocator& allocator);
        void Clear() noexcept;

    private:
        std::unordered_map<uint32_t, std::vector<ActorInstanceData>> m_batches;
    };
}

