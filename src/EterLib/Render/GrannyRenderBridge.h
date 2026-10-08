#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>

#ifndef TEST_MOCK_D3D9
#include <d3d9.h>
#else
struct D3DMATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;

        };
        float m[4][4];
    };
};
#endif

namespace EterLib::Render
{
    class RenderQueue;
    class LinearFrameAllocator;

    struct ActorDrawCommand
    {
        uint32_t meshId;
        const D3DMATRIX* instances;
        size_t instanceCount;
    };

    class GrannyMeshBatcher
    {
    public:
        void AddInstance(uint32_t meshId, const D3DMATRIX& world)
        {
            m_batches[meshId].push_back(world);
        }

        void Clear() noexcept
        {
            m_batches.clear();
        }

        const std::unordered_map<uint32_t, std::vector<D3DMATRIX>>& GetBatches() const noexcept
        {
            return m_batches;
        }

    private:
        std::unordered_map<uint32_t, std::vector<D3DMATRIX>> m_batches;
    };

    class GrannyRenderBridge
    {
    public:
        GrannyRenderBridge() = default;
        ~GrannyRenderBridge() = default;

        void SubmitModel(uint32_t meshId, const D3DMATRIX& world, RenderQueue& queue, LinearFrameAllocator& alloc);
        void FlushPending(RenderQueue& queue, LinearFrameAllocator& alloc);

    private:
        GrannyMeshBatcher m_batcher;
    };
} // namespace EterLib::Render

