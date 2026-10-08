#pragma once

#include <vector>
#include <cstdint>

#ifndef IS_TESTING
#include <d3d9.h>
#else
struct IDirect3DTexture9 {};
using LPDIRECT3DTEXTURE9 = IDirect3DTexture9*;
#endif

#include "RenderQueue.h"
#include "LinearFrameAllocator.h"
#include "../GrpBase.h"

namespace EterLib::Render
{
    class FontGlyphBatcher
    {
    public:
        FontGlyphBatcher() = default;
        ~FontGlyphBatcher() = default;

        FontGlyphBatcher(const FontGlyphBatcher&) = delete;
        FontGlyphBatcher& operator=(const FontGlyphBatcher&) = delete;
        FontGlyphBatcher(FontGlyphBatcher&&) = default;
        FontGlyphBatcher& operator=(FontGlyphBatcher&&) = default;

        void AddGlyph(float screenX, float screenY, float u0, float v0, float u1, float v1, uint32_t color);
        void Submit(RenderQueue& queue, LinearFrameAllocator& alloc, LPDIRECT3DTEXTURE9 fontTexture);
        void Clear() noexcept;

        // Exposed for testing
        const std::vector<TVertex>& GetVertices() const noexcept { return m_vertices; }

    private:
        std::vector<TVertex> m_vertices;
    };
}

