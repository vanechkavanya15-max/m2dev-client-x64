#pragma once

#include <cstdint>
#include <vector>

namespace EterLib::Render
{
    class RenderQueue;
    class LinearFrameAllocator;

    struct UIPrimitiveVertex
    {
        float x, y, z;
        uint32_t color;
        float u, v;
    };

    class UIPrimitiveBatcher
    {
    public:
        UIPrimitiveBatcher() = default;
        ~UIPrimitiveBatcher() = default;

        UIPrimitiveBatcher(const UIPrimitiveBatcher&) = delete;
        UIPrimitiveBatcher& operator=(const UIPrimitiveBatcher&) = delete;
        UIPrimitiveBatcher(UIPrimitiveBatcher&&) = default;
        UIPrimitiveBatcher& operator=(UIPrimitiveBatcher&&) = default;

        void AddRect(float x, float y, float w, float h, uint32_t color);
        void AddLine(float x1, float y1, float x2, float y2, uint32_t color);

        void DrawBar(float x, float y, float w, float h, uint32_t color) { AddRect(x, y, w, h, color); }
        void DrawBox(float x, float y, float w, float h, uint32_t color);

        void Submit(RenderQueue& queue, LinearFrameAllocator& alloc);
        void Clear() noexcept;

    private:
        std::vector<UIPrimitiveVertex> m_lines;
        std::vector<UIPrimitiveVertex> m_rects;
    };

} // namespace EterLib::Render

