#include "GeometryBufferRegistry.h"

namespace EterLib::Render
{
    GeometryBufferRegistry::GeometryBufferRegistry() noexcept
    {
        Invalidate();
    }

    bool GeometryBufferRegistry::BindVertexBuffer(LPDIRECT3DDEVICE9 dev, UINT stream, LPDIRECT3DVERTEXBUFFER9 vb, UINT offset, UINT stride) noexcept
    {
        if (!dev || stream >= MAX_STREAMS)
            return false;

        auto& state = m_streams[stream];
        if (state.vb == vb && state.offset == offset && state.stride == stride)
        {
            return true; // Already bound, skip
        }

        if (FAILED(dev->SetStreamSource(stream, vb, offset, stride)))
        {
            return false;
        }

        state.vb = vb;
        state.offset = offset;
        state.stride = stride;
        return true;
    }

    bool GeometryBufferRegistry::BindIndexBuffer(LPDIRECT3DDEVICE9 dev, LPDIRECT3DINDEXBUFFER9 ib) noexcept
    {
        if (!dev)
            return false;

        if (m_currentIndexBuffer == ib)
        {
            return true; // Already bound, skip
        }

        if (FAILED(dev->SetIndices(ib)))
        {
            return false;
        }

        m_currentIndexBuffer = ib;
        return true;
    }

    void GeometryBufferRegistry::Invalidate() noexcept
    {
        for (auto& stream : m_streams)
        {
            stream.vb = nullptr;
            stream.offset = 0;
            stream.stride = 0;
        }
        m_currentIndexBuffer = nullptr;
    }
}

