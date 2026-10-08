#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#define IS_TESTING
#include "../src/EterLib/Render/FontGlyphBatcher.h"

using namespace EterLib::Render;

TEST_CASE("FontGlyphBatcher - AddGlyph quad calculation and UV mapping")
{
    FontGlyphBatcher batcher;
    
    // Add a single glyph
    // screenX=10, screenY=20, u0=0, v0=0, u1=16, v1=16
    batcher.AddGlyph(10.0f, 20.0f, 0.0f, 0.0f, 16.0f, 16.0f, 0xFFFFFFFF);

    const auto& vertices = batcher.GetVertices();
    
    REQUIRE(vertices.size() == 6); // 2 triangles = 6 vertices

    // Triangle 1
    // v0: (screenX, screenY)
    CHECK(vertices[0].x == 10.0f);
    CHECK(vertices[0].y == 20.0f);
    CHECK(vertices[0].u == 0.0f);
    CHECK(vertices[0].v == 0.0f);
    CHECK(vertices[0].color == 0xFFFFFFFF);

    // v1: (screenX, screenY + height) -> height = v1 - v0 = 16
    CHECK(vertices[1].x == 10.0f);
    CHECK(vertices[1].y == 36.0f);
    CHECK(vertices[1].u == 0.0f);
    CHECK(vertices[1].v == 16.0f);

    // v2: (screenX + width, screenY) -> width = u1 - u0 = 16
    CHECK(vertices[2].x == 26.0f);
    CHECK(vertices[2].y == 20.0f);
    CHECK(vertices[2].u == 16.0f);
    CHECK(vertices[2].v == 0.0f);

    // Triangle 2
    // v3: (screenX + width, screenY)
    CHECK(vertices[3].x == 26.0f);
    CHECK(vertices[3].y == 20.0f);
    CHECK(vertices[3].u == 16.0f);
    CHECK(vertices[3].v == 0.0f);

    // v4: (screenX, screenY + height)
    CHECK(vertices[4].x == 10.0f);
    CHECK(vertices[4].y == 36.0f);
    CHECK(vertices[4].u == 0.0f);
    CHECK(vertices[4].v == 16.0f);

    // v5: (screenX + width, screenY + height)
    CHECK(vertices[5].x == 26.0f);
    CHECK(vertices[5].y == 36.0f);
    CHECK(vertices[5].u == 16.0f);
    CHECK(vertices[5].v == 16.0f);
}

TEST_CASE("FontGlyphBatcher - Clear")
{
    FontGlyphBatcher batcher;
    batcher.AddGlyph(0, 0, 0, 0, 10, 10, 0);
    REQUIRE(batcher.GetVertices().size() == 6);
    
    batcher.Clear();
    CHECK(batcher.GetVertices().empty());
}

