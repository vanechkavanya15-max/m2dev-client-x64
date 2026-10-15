#include "PythonGraphicModernBridge.h"

namespace EterLib::Render {

PythonGraphicModernBridge::PythonGraphicModernBridge(UIPrimitiveBatcher* primitiveBatcher, FontGlyphBatcher* fontBatcher)
    : m_primitiveBatcher(primitiveBatcher), m_fontBatcher(fontBatcher) {
}

void PythonGraphicModernBridge::RouteRenderBox(float x, float y, float w, float h, uint32_t color) {
    if (m_primitiveBatcher) {
        m_primitiveBatcher->DrawBox(x, y, w, h, color);
    }
}

void PythonGraphicModernBridge::RouteRenderBar(float x, float y, float w, float h, uint32_t color) {
    if (m_primitiveBatcher) {
        m_primitiveBatcher->DrawBar(x, y, w, h, color);
    }
}

void PythonGraphicModernBridge::SetDiffuseColor(uint32_t color) {
    m_currentDiffuseColor = color;
}

} // namespace EterLib::Render

