
#include "doctest.h"
#include "../src/EterLib/Render/PythonGraphicModernBridge.h"

using namespace EterLib::Render;

class MockUIPrimitiveBatcher : public UIPrimitiveBatcher {
public:
    int boxCalls = 0;
    int barCalls = 0;

    void DrawBox(float x, float y, float w, float h, uint32_t color) override {
        boxCalls++;
    }

    void DrawBar(float x, float y, float w, float h, uint32_t color) override {
        barCalls++;
    }
};

class MockFontGlyphBatcher : public FontGlyphBatcher {
};

TEST_CASE("PythonGraphicModernBridge routes correctly without GPU hanging") {
    MockUIPrimitiveBatcher primBatcher;
    MockFontGlyphBatcher fontBatcher;

    PythonGraphicModernBridge bridge(&primBatcher, &fontBatcher);

    bridge.SetDiffuseColor(0xFF00FF00);
    
    bridge.RouteRenderBox(10.0f, 20.0f, 100.0f, 50.0f, 0xFF00FF00);
    bridge.RouteRenderBar(15.0f, 25.0f, 90.0f, 40.0f, 0xFF00FF00);

    CHECK(primBatcher.boxCalls == 1);
    CHECK(primBatcher.barCalls == 1);
}

