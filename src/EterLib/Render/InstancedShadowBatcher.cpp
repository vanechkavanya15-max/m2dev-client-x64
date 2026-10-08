#include "InstancedShadowBatcher.h"
#include "RenderQueue.h"
#include "LinearFrameAllocator.h"
#include "DrawUserPrimitiveCommand.h"
#include <algorithm>

namespace EterLib::Render
{
    void InstancedShadowBatcher::AddShadow(float x, float y, float z, float radius, float alpha)
    {
        // Calculate ARGB color, RGB = 0, A = alpha
        uint8_t a = static_cast<uint8_t>(std::clamp(alpha * 255.0f, 0.0f, 255.0f));
        uint32_t color = (static_cast<uint32_t>(a) << 24) | 0x00000000;

        // Quad vertices for a shadow in XY plane, centered at (x, y, z)
        // using TriangleList, so we need 6 vertices for 2 triangles
        
        float xMin = x - radius;
        float xMax = x + radius;
        float yMin = y - radius;
        float yMax = y + radius;

        // Triangle 1
        m_vertices.push_back({xMin, yMin, z, color, 0.0f, 0.0f});
        m_vertices.push_back({xMax, yMin, z, color, 1.0f, 0.0f});
        m_vertices.push_back({xMin, yMax, z, color, 0.0f, 1.0f});

        // Triangle 2
        m_vertices.push_back({xMax, yMin, z, color, 1.0f, 0.0f});
        m_vertices.push_back({xMax, yMax, z, color, 1.0f, 1.0f});
        m_vertices.push_back({xMin, yMax, z, color, 0.0f, 1.0f});
    }

    void InstancedShadowBatcher::Submit(RenderQueue& queue, LinearFrameAllocator& allocator)
    {
        if (m_vertices.empty())
        {
            return;
        }

        UINT primitiveCount = static_cast<UINT>(m_vertices.size() / 3);

        auto* cmd = DrawUserPrimitiveCommand::Allocate(
            allocator, 
            D3DPT_TRIANGLELIST, 
            primitiveCount, 
            m_vertices.data(), 
            sizeof(Vertex)
        );

        if (cmd)
        {
            RenderSortKey key;
            key.value = static_cast<uint64_t>(Pass::AlphaBlend) << 56;
            queue.Submit(key, cmd, CommandType::Draw);
        }

        Clear();
    }

    void InstancedShadowBatcher::Clear() noexcept
    {
        m_vertices.clear();
    }
}

