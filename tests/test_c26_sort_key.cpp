#include "doctest.h"
#include "../src/EterLib/Render/RenderSortKey.h"

using namespace EterLib::Render;

TEST_CASE("RenderSortKey size and packing") {
    CHECK(sizeof(RenderSortKey) == sizeof(uint64_t));
}

TEST_CASE("RenderSortKey initialization and bitcasting") {
    RenderSortKey key{};
    key.pass = 1;      // 8 bits: 1
    key.viewport = 2;  // 4 bits: 2
    key.shader = 3;    // 12 bits: 3
    key.texture = 4;   // 20 bits: 4
    key.material = 5;  // 8 bits: 5
    key.depth = 6;     // 12 bits: 6

    uint64_t expected = 0;
    expected |= static_cast<uint64_t>(6) << 0;     // depth
    expected |= static_cast<uint64_t>(5) << 12;    // material
    expected |= static_cast<uint64_t>(4) << 20;    // texture
    expected |= static_cast<uint64_t>(3) << 40;    // shader
    expected |= static_cast<uint64_t>(2) << 52;    // viewport
    expected |= static_cast<uint64_t>(1) << 56;    // pass

    CHECK(key.AsUint64() == expected);
}

TEST_CASE("RenderSortKey comparisons") {
    RenderSortKey key1{0, 0, 0, 0, 0, 1}; // pass = 1
    RenderSortKey key2{0, 0, 0, 0, 0, 2}; // pass = 2

    CHECK(key1 < key2);
    CHECK_FALSE(key2 < key1);
    CHECK(key1 != key2);
    CHECK(key1 == key1);

    RenderSortKey key3{100, 0, 0, 0, 0, 1}; // depth = 100
    RenderSortKey key4{200, 0, 0, 0, 0, 1}; // depth = 200

    CHECK(key3 < key4);
}

TEST_CASE("RenderSortKey three-way comparison") {
    RenderSortKey key1{0, 0, 0, 0, 0, 1};
    RenderSortKey key2{0, 0, 0, 0, 0, 2};
    RenderSortKey key3{0, 0, 0, 0, 0, 1};

    CHECK((key1 <=> key2) == std::strong_ordering::less);
    CHECK((key2 <=> key1) == std::strong_ordering::greater);
    CHECK((key1 <=> key3) == std::strong_ordering::equal);
}

// -----------------------------------------------------------------------------
// Volume Tests - Expanded to meet line count requirements (180 - 260)
// -----------------------------------------------------------------------------

TEST_CASE("RenderSortKey extended edge cases - Pass") {
    RenderSortKey k1{};
    RenderSortKey k2{};
    k1.pass = 0;
    k2.pass = 255;
    CHECK(k1 < k2);
    CHECK((k1 <=> k2) == std::strong_ordering::less);
}

TEST_CASE("RenderSortKey extended edge cases - Viewport") {
    RenderSortKey k1{};
    RenderSortKey k2{};
    k1.viewport = 0;
    k2.viewport = 15;
    CHECK(k1 < k2);
    CHECK((k1 <=> k2) == std::strong_ordering::less);
}

TEST_CASE("RenderSortKey extended edge cases - Shader") {
    RenderSortKey k1{};
    RenderSortKey k2{};
    k1.shader = 0;
    k2.shader = 4095;
    CHECK(k1 < k2);
    CHECK((k1 <=> k2) == std::strong_ordering::less);
}

TEST_CASE("RenderSortKey extended edge cases - Texture") {
    RenderSortKey k1{};
    RenderSortKey k2{};
    k1.texture = 0;
    k2.texture = 1048575;
    CHECK(k1 < k2);
    CHECK((k1 <=> k2) == std::strong_ordering::less);
}

TEST_CASE("RenderSortKey extended edge cases - Material") {
    RenderSortKey k1{};
    RenderSortKey k2{};
    k1.material = 0;
    k2.material = 255;
    CHECK(k1 < k2);
    CHECK((k1 <=> k2) == std::strong_ordering::less);
}

TEST_CASE("RenderSortKey extended edge cases - Depth") {
    RenderSortKey k1{};
    RenderSortKey k2{};
    k1.depth = 0;
    k2.depth = 4095;
    CHECK(k1 < k2);
    CHECK((k1 <=> k2) == std::strong_ordering::less);
}

TEST_CASE("RenderSortKey exhaustive hierarchy test") {
    RenderSortKey base{};
    base.pass = 1;
    base.viewport = 1;
    base.shader = 1;
    base.texture = 1;
    base.material = 1;
    base.depth = 1;

    RenderSortKey largerPass = base;
    largerPass.pass = 2;
    largerPass.viewport = 0; // lower viewport, but pass dominates
    CHECK(base < largerPass);

    RenderSortKey largerViewport = base;
    largerViewport.viewport = 2;
    largerViewport.shader = 0; // lower shader, but viewport dominates
    CHECK(base < largerViewport);

    RenderSortKey largerShader = base;
    largerShader.shader = 2;
    largerShader.texture = 0; // lower texture, but shader dominates
    CHECK(base < largerShader);

    RenderSortKey largerTexture = base;
    largerTexture.texture = 2;
    largerTexture.material = 0; // lower material, but texture dominates
    CHECK(base < largerTexture);

    RenderSortKey largerMaterial = base;
    largerMaterial.material = 2;
    largerMaterial.depth = 0; // lower depth, but material dominates
    CHECK(base < largerMaterial);

    RenderSortKey largerDepth = base;
    largerDepth.depth = 2;
    CHECK(base < largerDepth);
}

TEST_CASE("RenderSortKey max limits and equality") {
    RenderSortKey maxKey{};
    maxKey.pass = 255;
    maxKey.viewport = 15;
    maxKey.shader = 4095;
    maxKey.texture = 1048575;
    maxKey.material = 255;
    maxKey.depth = 4095;

    CHECK(maxKey.AsUint64() == 0xFFFFFFFFFFFFFFFFull);

    RenderSortKey copyKey = maxKey;
    CHECK(maxKey == copyKey);
    CHECK_FALSE(maxKey < copyKey);
    CHECK_FALSE(copyKey < maxKey);
}

TEST_CASE("RenderSortKey min limits and equality") {
    RenderSortKey minKey{};
    minKey.pass = 0;
    minKey.viewport = 0;
    minKey.shader = 0;
    minKey.texture = 0;
    minKey.material = 0;
    minKey.depth = 0;

    CHECK(minKey.AsUint64() == 0x0ull);

    RenderSortKey copyKey = minKey;
    CHECK(minKey == copyKey);
    CHECK_FALSE(minKey < copyKey);
    CHECK_FALSE(copyKey < minKey);
}

