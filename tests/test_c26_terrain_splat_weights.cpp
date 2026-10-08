#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#define TEST_MODE_DISABLE_STDAFX 1
#include "../src/EterLib/Render/TerrainSplatWeightMap.h"

using namespace EterLib::Render;

TEST_CASE("TerrainSplatWeightMap Initialization and Defaults") {
    TerrainSplatWeightMap weightMap;
    weightMap.Initialize(2, 2);

    auto rawData = weightMap.GetRawData();
    CHECK(rawData.size() == 4);

    for (uint32_t y = 0; y < 2; ++y) {
        for (uint32_t x = 0; x < 2; ++x) {
            uint32_t packed = weightMap.GetPackedWeight(x, y);
            CHECK(packed == 0);
        }
    }
}

TEST_CASE("TerrainSplatWeightMap Normalization to 255") {
    TerrainSplatWeightMap weightMap;
    weightMap.Initialize(1, 1);

    // Initial state: sumOther is 0, so setting layer 0 should put the rest into another layer (layer 1).
    weightMap.SetLayerWeight(0, 100);
    uint32_t packed1 = weightMap.GetPackedWeight(0, 0);
    
    uint8_t w0 = packed1 & 0xFF;
    uint8_t w1 = (packed1 >> 8) & 0xFF;
    uint8_t w2 = (packed1 >> 16) & 0xFF;
    uint8_t w3 = (packed1 >> 24) & 0xFF;

    CHECK(w0 == 100);
    CHECK(w0 + w1 + w2 + w3 == 255);

    // Set layer 1 to 50
    // w0 was 100, now remaining is 255 - 50 = 205.
    // sumOther is w0 (100) + w2 (0) + w3 (0) = 100.
    // w0 gets scaled: (100 * 205) / 100 = 205.
    weightMap.SetLayerWeight(1, 50);
    uint32_t packed2 = weightMap.GetPackedWeight(0, 0);
    
    w0 = packed2 & 0xFF;
    w1 = (packed2 >> 8) & 0xFF;
    w2 = (packed2 >> 16) & 0xFF;
    w3 = (packed2 >> 24) & 0xFF;

    CHECK(w1 == 50);
    CHECK(w0 == 205);
    CHECK(w2 == 0);
    CHECK(w3 == 0);
    CHECK(w0 + w1 + w2 + w3 == 255);

    // Set layer 2 to 255 (max weight)
    weightMap.SetLayerWeight(2, 255);
    uint32_t packed3 = weightMap.GetPackedWeight(0, 0);
    
    w0 = packed3 & 0xFF;
    w1 = (packed3 >> 8) & 0xFF;
    w2 = (packed3 >> 16) & 0xFF;
    w3 = (packed3 >> 24) & 0xFF;

    CHECK(w2 == 255);
    CHECK(w0 == 0);
    CHECK(w1 == 0);
    CHECK(w3 == 0);
    CHECK(w0 + w1 + w2 + w3 == 255);
}

TEST_CASE("TerrainSplatWeightMap Overflow Prevention and Integer Distribution") {
    TerrainSplatWeightMap weightMap;
    weightMap.Initialize(1, 1);
    
    // Distribute weights equally
    weightMap.SetLayerWeight(0, 85);
    weightMap.SetLayerWeight(1, 85);
    weightMap.SetLayerWeight(2, 85);
    
    uint32_t packed = weightMap.GetPackedWeight(0, 0);
    uint8_t w0 = packed & 0xFF;
    uint8_t w1 = (packed >> 8) & 0xFF;
    uint8_t w2 = (packed >> 16) & 0xFF;
    uint8_t w3 = (packed >> 24) & 0xFF;
    
    CHECK(w0 + w1 + w2 + w3 == 255);
    
    // Change w3, check if w0, w1, w2 still add up to 255 - w3
    weightMap.SetLayerWeight(3, 100);
    
    packed = weightMap.GetPackedWeight(0, 0);
    w0 = packed & 0xFF;
    w1 = (packed >> 8) & 0xFF;
    w2 = (packed >> 16) & 0xFF;
    w3 = (packed >> 24) & 0xFF;
    
    CHECK(w3 == 100);
    CHECK(w0 + w1 + w2 + w3 == 255);
}

