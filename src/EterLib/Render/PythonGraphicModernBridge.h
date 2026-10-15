#pragma once

#include <cstdint>

#include "UIPrimitiveBatcher.h"
#include "FontGlyphBatcher.h"

namespace EterLib::Render {

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

