#pragma once

#include <cstdint>

namespace EterLib::Render {

class UIPrimitiveBatcher {
public:
    virtual ~UIPrimitiveBatcher() = default;
    virtual void DrawBox(float x, float y, float w, float h, uint32_t color) = 0;
    virtual void DrawBar(float x, float y, float w, float h, uint32_t color) = 0;
};

class FontGlyphBatcher {
public:
    virtual ~FontGlyphBatcher() = default;
};

class PythonGraphicModernBridge {
public:
    PythonGraphicModernBridge(UIPrimitiveBatcher* primitiveBatcher, FontGlyphBatcher* fontBatcher);

    void RouteRenderBox(float x, float y, float w, float h, uint32_t color);
    void RouteRenderBar(float x, float y, float w, float h, uint32_t color);
    void SetDiffuseColor(uint32_t color);

private:
    UIPrimitiveBatcher* m_primitiveBatcher;
    FontGlyphBatcher* m_fontBatcher;
    uint32_t m_currentDiffuseColor = 0xFFFFFFFF;
};

} // namespace EterLib::Render

