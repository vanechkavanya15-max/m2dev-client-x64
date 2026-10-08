#include "FontGlyphBatcher.h"
#include "DrawUserPrimitiveCommand.h"
#include "SortKeyBuilder.h"

namespace EterLib::Render
{
    struct SetTextureCommand
    {
        LPDIRECT3DTEXTURE9 texture;

        void Execute(LPDIRECT3DDEVICE9 dev) const noexcept
        {
            if (dev)
            {
                dev->SetTexture(0, texture);
            }
        }
    };

    void FontGlyphBatcher::AddGlyph(float screenX, float screenY, float u0, float v0, float u1, float v1, uint32_t color)
    {
        float w = u1 - u0;
        float h = v1 - v0;

        TVertex v[6];
        
        // Triangle 1
        v[0].x = screenX;
        v[0].y = screenY;
        v[0].z = 0.0f;
        v[0].color = color;
        v[0].u = u0;
        v[0].v = v0;

        v[1].x = screenX;
        v[1].y = screenY + h;
        v[1].z = 0.0f;
        v[1].color = color;
        v[1].u = u0;
        v[1].v = v1;

        v[2].x = screenX + w;
        v[2].y = screenY;
        v[2].z = 0.0f;
        v[2].color = color;
        v[2].u = u1;
        v[2].v = v0;

        // Triangle 2
        v[3].x = screenX + w;
        v[3].y = screenY;
        v[3].z = 0.0f;
        v[3].color = color;
        v[3].u = u1;
        v[3].v = v0;

        v[4].x = screenX;
        v[4].y = screenY + h;
        v[4].z = 0.0f;
        v[4].color = color;
        v[4].u = u0;
        v[4].v = v1;

        v[5].x = screenX + w;
        v[5].y = screenY + h;
        v[5].z = 0.0f;
        v[5].color = color;
        v[5].u = u1;
        v[5].v = v1;

        m_vertices.insert(m_vertices.end(), std::begin(v), std::end(v));
    }

    void FontGlyphBatcher::Submit(RenderQueue& queue, LinearFrameAllocator& alloc, LPDIRECT3DTEXTURE9 fontTexture)
    {
        if (m_vertices.empty())
            return;

        RenderSortKey key = SortKeyBuilder().WithPass(static_cast<uint8_t>(Pass::UI)).Build();

        auto* setTexCmd = alloc.AllocateObject<SetTextureCommand>();
        if (setTexCmd)
        {
            setTexCmd->texture = fontTexture;
            queue.Submit(key, setTexCmd, CommandType::StateChange);
        }

        UINT primCount = static_cast<UINT>(m_vertices.size() / 3);
        auto* drawCmd = DrawUserPrimitiveCommand::Allocate(
            alloc, 
            D3DPT_TRIANGLELIST, 
            primCount, 
            m_vertices.data(), 
            sizeof(TVertex)
        );

        if (drawCmd)
        {
            queue.Submit(key, drawCmd, CommandType::Draw);
        }

        Clear();
    }

    void FontGlyphBatcher::Clear() noexcept
    {
        m_vertices.clear();
    }
}

