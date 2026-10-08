#include <gtest/gtest.h>
#include "../src/EterLib/Render/SortKeyBuilder.h"

using namespace EterLib::Render;

class SortKeyBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup if needed
    }

    void TearDown() override {
        // Test teardown if needed
    }
};

TEST_F(SortKeyBuilderTest, DefaultBuilderProducesZeroKey) {
    SortKeyBuilder builder;
    RenderSortKey key = builder.Build();
    EXPECT_EQ(key.value, 0ULL);
}

TEST_F(SortKeyBuilderTest, WithPassSetsCorrectBits) {
    SortKeyBuilder builder;
    RenderSortKey key = builder.WithPass(0xA5).Build();
    EXPECT_EQ(key.value, 0xA5ULL << 56);
}

TEST_F(SortKeyBuilderTest, WithPassTruncatesTo8Bits) {
    SortKeyBuilder builder;
    // 0x1A5 & 0xFF = 0xA5
    RenderSortKey key = builder.WithPass(0xA5).Build();
    EXPECT_EQ(key.value, 0xA5ULL << 56);
}

TEST_F(SortKeyBuilderTest, WithViewportSetsCorrectBits) {
    SortKeyBuilder builder;
    RenderSortKey key = builder.WithViewport(0xA).Build();
    EXPECT_EQ(key.value, 0xAULL << 52);
}

TEST_F(SortKeyBuilderTest, WithViewportTruncatesTo4Bits) {
    SortKeyBuilder builder;
    // 0x1A & 0xF = 0xA
    RenderSortKey key = builder.WithViewport(0x1A).Build();
    EXPECT_EQ(key.value, 0xAULL << 52);
}

TEST_F(SortKeyBuilderTest, WithDepthSetsCorrectBits) {
    SortKeyBuilder builder;
    // 0.5f * 4095 = 2047.5 -> 2047 (0x7FF)
    RenderSortKey key = builder.WithDepth(0.5f).Build();
    EXPECT_EQ(key.value, 0x7FFULL << 40);
}

TEST_F(SortKeyBuilderTest, WithDepthClampsToZero) {
    SortKeyBuilder builder;
    RenderSortKey key = builder.WithDepth(-1.0f).Build();
    EXPECT_EQ(key.value, 0ULL << 40);
}

TEST_F(SortKeyBuilderTest, WithDepthClampsToOne) {
    SortKeyBuilder builder;
    // 1.0f * 4095 = 4095 (0xFFF)
    RenderSortKey key = builder.WithDepth(2.0f).Build();
    EXPECT_EQ(key.value, 0xFFFULL << 40);
}

TEST_F(SortKeyBuilderTest, WithDepthReversesForTransparent) {
    SortKeyBuilder builder;
    // 0.0f normal -> 0
    // 0.0f reversed -> 4095 (0xFFF)
    RenderSortKey key = builder.WithDepth(0.0f, true).Build();
    EXPECT_EQ(key.value, 0xFFFULL << 40);
}

TEST_F(SortKeyBuilderTest, WithDepthReversesForTransparentMax) {
    SortKeyBuilder builder;
    // 1.0f normal -> 4095
    // 1.0f reversed -> 0
    RenderSortKey key = builder.WithDepth(1.0f, true).Build();
    EXPECT_EQ(key.value, 0ULL << 40);
}

TEST_F(SortKeyBuilderTest, WithShaderSetsCorrectBits) {
    SortKeyBuilder builder;
    RenderSortKey key = builder.WithShader(0xABCD).Build();
    EXPECT_EQ(key.value, 0xABCDULL << 24);
}

TEST_F(SortKeyBuilderTest, WithMaterialSetsCorrectBits) {
    SortKeyBuilder builder;
    RenderSortKey key = builder.WithMaterial(0xCD).Build();
    EXPECT_EQ(key.value, 0xCDULL << 16);
}

TEST_F(SortKeyBuilderTest, WithTextureSetsCorrectBits) {
    SortKeyBuilder builder;
    RenderSortKey key = builder.WithTexture(0x1234).Build();
    EXPECT_EQ(key.value, 0x1234ULL);
}

TEST_F(SortKeyBuilderTest, WithTextureTruncatesTo16Bits) {
    SortKeyBuilder builder;
    RenderSortKey key = builder.WithTexture(0x991234).Build();
    EXPECT_EQ(key.value, 0x1234ULL);
}

TEST_F(SortKeyBuilderTest, FluentAPIChaining) {
    RenderSortKey key = SortKeyBuilder()
        .WithPass(0x12)
        .WithViewport(0x3)
        .WithDepth(0.5f) // 0x7FF
        .WithShader(0x4567)
        .WithMaterial(0x89)
        .WithTexture(0xABCD)
        .Build();

    uint64_t expected = (0x12ULL << 56) |
                        (0x3ULL << 52) |
                        (0x7FFULL << 40) |
                        (0x4567ULL << 24) |
                        (0x89ULL << 16) |
                        (0xABCDULL);

    EXPECT_EQ(key.value, expected);
}

TEST_F(SortKeyBuilderTest, SortingOpaqueFrontToBack) {
    // Opaque rendering typically sorts front-to-back to minimize overdraw (early-Z).
    // Smaller depth values (closer) should have a smaller sort key to be rendered first.
    RenderSortKey nearKey = SortKeyBuilder().WithDepth(0.1f).Build();
    RenderSortKey farKey = SortKeyBuilder().WithDepth(0.9f).Build();

    EXPECT_LT(nearKey, farKey);
}

TEST_F(SortKeyBuilderTest, SortingTransparentBackToFront) {
    // Transparent rendering typically sorts back-to-front for proper alpha blending.
    // Larger depth values (further) should have a smaller sort key to be rendered first.
    // We achieve this by reversing the depth.
    RenderSortKey nearKey = SortKeyBuilder().WithDepth(0.1f, true).Build();
    RenderSortKey farKey = SortKeyBuilder().WithDepth(0.9f, true).Build();

    // Far key (reversed: smaller int) should be less than near key (reversed: larger int)
    EXPECT_LT(farKey, nearKey);
}

TEST_F(SortKeyBuilderTest, SortingByPassPriority) {
    RenderSortKey pass1Key = SortKeyBuilder().WithPass(1).WithDepth(0.9f).Build();
    RenderSortKey pass2Key = SortKeyBuilder().WithPass(2).WithDepth(0.1f).Build();

    // Even though pass 2 is closer, pass 1 should render first because pass has higher priority in the key.
    EXPECT_LT(pass1Key, pass2Key);
}

TEST_F(SortKeyBuilderTest, SortingByMultipleCriteria) {
    RenderSortKey key1 = SortKeyBuilder()
        .WithPass(1)
        .WithDepth(0.5f)
        .WithShader(10)
        .Build();
        
    RenderSortKey key2 = SortKeyBuilder()
        .WithPass(1)
        .WithDepth(0.5f)
        .WithShader(20)
        .Build();
        
    EXPECT_LT(key1, key2);
}

TEST_F(SortKeyBuilderTest, OverwritingState) {
    RenderSortKey key = SortKeyBuilder()
        .WithPass(1)
        .WithPass(2)
        .WithTexture(0x1111)
        .WithTexture(0x2222)
        .Build();

    EXPECT_EQ(key.value, (2ULL << 56) | 0x2222ULL);
}

TEST_F(SortKeyBuilderTest, EdgeCaseMinValues) {
    RenderSortKey key = SortKeyBuilder()
        .WithPass(0)
        .WithViewport(0)
        .WithDepth(0.0f)
        .WithShader(0)
        .WithMaterial(0)
        .WithTexture(0)
        .Build();
        
    EXPECT_EQ(key.value, 0ULL);
}

TEST_F(SortKeyBuilderTest, EdgeCaseMaxValues) {
    RenderSortKey key = SortKeyBuilder()
        .WithPass(0xFF)
        .WithViewport(0xF)
        .WithDepth(1.0f)
        .WithShader(0xFFFF)
        .WithMaterial(0xFF)
        .WithTexture(0xFFFF)
        .Build();
        
    uint64_t expected = (0xFFULL << 56) |
                        (0xFULL << 52) |
                        (0xFFFULL << 40) |
                        (0xFFFFULL << 24) |
                        (0xFFULL << 16) |
                        (0xFFFFULL);
                        
    EXPECT_EQ(key.value, expected);
}

