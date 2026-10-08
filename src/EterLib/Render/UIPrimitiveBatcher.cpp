#include "UIPrimitiveBatcher.h"
#include "LinearFrameAllocator.h"
#include "RenderQueue.h"
#include "DrawUserPrimitiveCommand.h"

namespace EterLib::Render
{
    void UIPrimitiveBatcher::AddRect(float x, float y, float w, float h, uint32_t color)
    {
        // Add 2 triangles (6 vertices) for a rect
        // Triangle 1
        m_rects.push_back({x, y, 0.0f, color, 0.0f, 0.0f});
        m_rects.push_back({x + w, y, 0.0f, color, 0.0f, 0.0f});
        m_rects.push_back({x, y + h, 0.0f, color, 0.0f, 0.0f});
        
        // Triangle 2
        m_rects.push_back({x + w, y, 0.0f, color, 0.0f, 0.0f});
        m_rects.push_back({x + w, y + h, 0.0f, color, 0.0f, 0.0f});
        m_rects.push_back({x, y + h, 0.0f, color, 0.0f, 0.0f});
    }

    void UIPrimitiveBatcher::AddLine(float x1, float y1, float x2, float y2, uint32_t color)
    {
        m_lines.push_back({x1, y1, 0.0f, color, 0.0f, 0.0f});
        m_lines.push_back({x2, y2, 0.0f, color, 0.0f, 0.0f});
    }

    void UIPrimitiveBatcher::Submit(RenderQueue& queue, LinearFrameAllocator& alloc)
    {
        if (!m_rects.empty())
        {
            UINT primitiveCount = static_cast<UINT>(m_rects.size() / 3);
            auto* cmd = DrawUserPrimitiveCommand::Allocate(
                alloc, 
                D3DPT_TRIANGLELIST, 
                primitiveCount, 
                m_rects.data(), 
                static_cast<UINT>(sizeof(UIPrimitiveVertex))
            );

            if (cmd)
            {
                RenderSortKey key{0}; // Set UI sort key to 0 or similar
                queue.Submit(key, cmd, CommandType::Draw);
            }
            m_rects.clear();
        }

        if (!m_lines.empty())
        {
            UINT primitiveCount = static_cast<UINT>(m_lines.size() / 2);
            auto* cmd = DrawUserPrimitiveCommand::Allocate(
                alloc, 
                D3DPT_LINELIST, 
                primitiveCount, 
                m_lines.data(), 
                static_cast<UINT>(sizeof(UIPrimitiveVertex))
            );

            if (cmd)
            {
                RenderSortKey key{0};
                queue.Submit(key, cmd, CommandType::Draw);
            }
            m_lines.clear();
        }
    }
} // namespace EterLib::Render

