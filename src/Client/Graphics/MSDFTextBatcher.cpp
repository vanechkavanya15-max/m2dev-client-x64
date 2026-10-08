#include "MSDFTextBatcher.h"

#include <d3d9.h>
#include <cstring>

namespace Client::Graphics
{

MSDFTextBatcher::MSDFTextBatcher(const MSDFFontAtlas* pFontAtlas) noexcept
    : m_pFontAtlas(pFontAtlas)
{
}

void MSDFTextBatcher::AddText(
    std::string_view text,
    float x,
    float y,
    float z,
    uint32_t color,
    uint32_t outlineColor,
    float outlineWidth,
    float pxRange,
    float scale)
{
    if (!m_pFontAtlas || text.empty())
    {
        return;
    }

    ++m_batchedStringCount;

    float curX = x;
    float curY = y;
    size_t offset = 0;

    while (offset < text.size())
    {
        const char32_t cp = DecodeNextUtf8Codepoint(text, offset);
        if (cp == 0)
        {
            break;
        }

        if (cp == U'\r')
        {
            continue;
        }

        if (cp == U'\n')
        {
            curY += m_pFontAtlas->GetLineHeight() * scale;
            curX = x;
            continue;
        }

        const auto* glyph = m_pFontAtlas->GetGlyph(cp);
        if (!glyph)
        {
            continue;
        }

        if (glyph->width > 0.0f && glyph->height > 0.0f)
        {
            // Ochrona przed przekroczeniem zakresu 16-bitowego bufora indeksow (uint16_t max 65535)
            if (m_vertices.size() + 4 > 65532)
            {
                break;
            }

            const float x0 = curX + glyph->offsetX * scale;
            const float y0 = curY + glyph->offsetY * scale;
            const float x1 = x0 + glyph->width * scale;
            const float y1 = y0 + glyph->height * scale;

            const auto baseVertex = static_cast<uint16_t>(m_vertices.size());

            // 4 wierzcholki tworzace prostokat glifu
            m_vertices.push_back(MSDFVertex{
                .x = x0,
                .y = y0,
                .z = z,
                .u = glyph->u0,
                .v = glyph->v0,
                .color = color,
                .outlineColor = outlineColor,
                .outlineWidth = outlineWidth,
                .pxRange = pxRange
            });

            m_vertices.push_back(MSDFVertex{
                .x = x1,
                .y = y0,
                .z = z,
                .u = glyph->u1,
                .v = glyph->v0,
                .color = color,
                .outlineColor = outlineColor,
                .outlineWidth = outlineWidth,
                .pxRange = pxRange
            });

            m_vertices.push_back(MSDFVertex{
                .x = x1,
                .y = y1,
                .z = z,
                .u = glyph->u1,
                .v = glyph->v1,
                .color = color,
                .outlineColor = outlineColor,
                .outlineWidth = outlineWidth,
                .pxRange = pxRange
            });

            m_vertices.push_back(MSDFVertex{
                .x = x0,
                .y = y1,
                .z = z,
                .u = glyph->u0,
                .v = glyph->v1,
                .color = color,
                .outlineColor = outlineColor,
                .outlineWidth = outlineWidth,
                .pxRange = pxRange
            });

            // 6 indeksow na quad: dwa trojkaty o ukladzie (0,1,2, 2,3,0)
            m_indices.push_back(static_cast<uint16_t>(baseVertex + 0));
            m_indices.push_back(static_cast<uint16_t>(baseVertex + 1));
            m_indices.push_back(static_cast<uint16_t>(baseVertex + 2));
            m_indices.push_back(static_cast<uint16_t>(baseVertex + 2));
            m_indices.push_back(static_cast<uint16_t>(baseVertex + 3));
            m_indices.push_back(static_cast<uint16_t>(baseVertex + 0));

            ++m_batchedGlyphCount;
        }

        curX += glyph->advanceX * scale;
    }
}

bool MSDFTextBatcher::Flush(DynamicGeometryRingBuffer* pRingBuffer, IDirect3DDevice9* pDevice)
{
    if (m_vertices.empty())
    {
        m_drawCallsCount = 0;
        m_lastFlushInfo = {};
        return false;
    }

    if (!pRingBuffer || !pRingBuffer->IsInitialized())
    {
        return false;
    }

    const uint32_t vertexCount = static_cast<uint32_t>(m_vertices.size());
    const uint32_t indexCount = static_cast<uint32_t>(m_indices.size());

    // Alokacja w buforze pierscieniowym z uzyciem D3DLOCK_NOOVERWRITE (lub DISCARD przy wrap-around)
    auto vertexAlloc = pRingBuffer->AllocateVertices(vertexCount, sizeof(MSDFVertex));
    if (!vertexAlloc.pData)
    {
        return false;
    }

    auto indexAlloc = pRingBuffer->AllocateIndices(indexCount);
    if (!indexAlloc.pData)
    {
        pRingBuffer->UnlockVertices();
        return false;
    }

    // Bezposredni transfer geometrii do pamieci bufora
    std::memcpy(vertexAlloc.pData, m_vertices.data(), vertexCount * sizeof(MSDFVertex));
    std::memcpy(indexAlloc.pData, m_indices.data(), indexCount * sizeof(uint16_t));

    // Odblokowanie zasobow sprzetowych / bufora emulacji
    pRingBuffer->UnlockVertices();
    pRingBuffer->UnlockIndices();

    // Rysowanie calej zagregowanej partii w jednym wywolaniu DrawIndexedPrimitive
    if (pDevice)
    {
        pDevice->SetStreamSource(0, pRingBuffer->GetVertexBuffer(), 0, sizeof(MSDFVertex));
        pDevice->SetIndices(pRingBuffer->GetIndexBuffer());

        const UINT primitiveCount = indexCount / 3;
        pDevice->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            static_cast<INT>(vertexAlloc.baseVertex),
            0,
            vertexCount,
            static_cast<UINT>(indexAlloc.startIndex),
            primitiveCount
        );
    }

    m_drawCallsCount = 1;

    m_lastFlushInfo = LastFlushInfo{
        .vertexAlloc = vertexAlloc,
        .indexAlloc = indexAlloc,
        .vertexCount = vertexCount,
        .indexCount = indexCount,
        .success = true
    };

    // Czyszczenie lokalnych list na kolejna partie w ramce
    m_vertices.clear();
    m_indices.clear();

    return true;
}

void MSDFTextBatcher::ResetFrame() noexcept
{
    m_batchedStringCount = 0;
    m_batchedGlyphCount = 0;
    m_drawCallsCount = 0;
    m_vertices.clear();
    m_indices.clear();
    m_lastFlushInfo = {};
}

} // namespace Client::Graphics
