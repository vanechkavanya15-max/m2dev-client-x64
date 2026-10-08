#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <random>
#include "../src/EterLib/Render/RenderStateDeduplicator.h"

using namespace EterLib::Render;

class RenderStateDeduplicatorTest : public ::testing::Test
{
protected:
    RenderStateDeduplicator deduplicator;
    
    std::vector<uint32_t> shaderBinds;
    std::vector<uint32_t> textureBinds;
    std::vector<uint32_t> materialBinds;
    std::vector<RenderItem> drawnItems;

    void SetUp() override
    {
        deduplicator.ResetStats();
        shaderBinds.clear();
        textureBinds.clear();
        materialBinds.clear();
        drawnItems.clear();
    }

    auto getBindShader() {
        return [this](uint32_t id) { shaderBinds.push_back(id); };
    }

    auto getBindTexture() {
        return [this](uint32_t id) { textureBinds.push_back(id); };
    }

    auto getBindMaterial() {
        return [this](uint32_t id) { materialBinds.push_back(id); };
    }

    auto getDrawCallback() {
        return [this](const RenderItem& item) { drawnItems.push_back(item); };
    }
    
    std::vector<RenderItem> GenerateSortedQueue(size_t size, int numShaders, int numTextures, int numMaterials)
    {
        std::vector<RenderItem> queue(size);
        std::mt19937 gen(42); // Fixed seed for reproducibility
        std::uniform_int_distribution<> shaderDist(1, numShaders);
        std::uniform_int_distribution<> textureDist(1, numTextures);
        std::uniform_int_distribution<> materialDist(1, numMaterials);
        
        for(size_t i = 0; i < size; ++i)
        {
            queue[i].sortKey.Shader = shaderDist(gen);
            queue[i].sortKey.Texture = textureDist(gen);
            queue[i].sortKey.Material = materialDist(gen);
            queue[i].commandData = reinterpret_cast<void*>(i + 1);
        }
        
        std::sort(queue.begin(), queue.end(), [](const RenderItem& a, const RenderItem& b) {
            if (a.sortKey.Shader != b.sortKey.Shader) return a.sortKey.Shader < b.sortKey.Shader;
            if (a.sortKey.Texture != b.sortKey.Texture) return a.sortKey.Texture < b.sortKey.Texture;
            return a.sortKey.Material < b.sortKey.Material;
        });
        
        return queue;
    }
};

TEST_F(RenderStateDeduplicatorTest, EmptyQueueDoesNothing)
{
    std::vector<RenderItem> queue;
    
    deduplicator.ProcessQueue(queue, getBindShader(), getBindTexture(), getBindMaterial(), getDrawCallback());
    
    EXPECT_TRUE(shaderBinds.empty());
    EXPECT_TRUE(textureBinds.empty());
    EXPECT_TRUE(materialBinds.empty());
    EXPECT_TRUE(drawnItems.empty());
    
    const auto& stats = deduplicator.GetStats();
    EXPECT_EQ(stats.totalItems, 0);
    EXPECT_EQ(stats.totalSavedSwitches(), 0);
}

TEST_F(RenderStateDeduplicatorTest, SingleItemBindsAllStates)
{
    std::vector<RenderItem> queue(1);
    queue[0].sortKey = {1, 2, 3};
    queue[0].commandData = reinterpret_cast<void*>(100);
    
    deduplicator.ProcessQueue(queue, getBindShader(), getBindTexture(), getBindMaterial(), getDrawCallback());
    
    ASSERT_EQ(shaderBinds.size(), 1);
    EXPECT_EQ(shaderBinds[0], 1);
    
    ASSERT_EQ(textureBinds.size(), 1);
    EXPECT_EQ(textureBinds[0], 2);
    
    ASSERT_EQ(materialBinds.size(), 1);
    EXPECT_EQ(materialBinds[0], 3);
    
    ASSERT_EQ(drawnItems.size(), 1);
    EXPECT_EQ(drawnItems[0].commandData, reinterpret_cast<void*>(100));
    
    const auto& stats = deduplicator.GetStats();
    EXPECT_EQ(stats.totalItems, 1);
    EXPECT_EQ(stats.actualShaderSwitches, 1);
    EXPECT_EQ(stats.actualTextureSwitches, 1);
    EXPECT_EQ(stats.actualMaterialSwitches, 1);
    EXPECT_EQ(stats.totalSavedSwitches(), 0);
}

TEST_F(RenderStateDeduplicatorTest, IdenticalItemsDeduplicateCompletely)
{
    std::vector<RenderItem> queue(10);
    for (int i = 0; i < 10; ++i) {
        queue[i].sortKey = {5, 5, 5};
        queue[i].commandData = reinterpret_cast<void*>(i + 1);
    }
    
    deduplicator.ProcessQueue(queue, getBindShader(), getBindTexture(), getBindMaterial(), getDrawCallback());
    
    EXPECT_EQ(shaderBinds.size(), 1);
    EXPECT_EQ(textureBinds.size(), 1);
    EXPECT_EQ(materialBinds.size(), 1);
    EXPECT_EQ(drawnItems.size(), 10);
    
    const auto& stats = deduplicator.GetStats();
    EXPECT_EQ(stats.totalItems, 10);
    EXPECT_EQ(stats.actualShaderSwitches, 1);
    EXPECT_EQ(stats.actualTextureSwitches, 1);
    EXPECT_EQ(stats.actualMaterialSwitches, 1);
    EXPECT_EQ(stats.savedShaderSwitches(), 9);
    EXPECT_EQ(stats.savedTextureSwitches(), 9);
    EXPECT_EQ(stats.savedMaterialSwitches(), 9);
    EXPECT_EQ(stats.totalSavedSwitches(), 27);
    EXPECT_DOUBLE_EQ(stats.getReductionPercentage(), 90.0);
}

TEST_F(RenderStateDeduplicatorTest, PartialDeduplication)
{
    std::vector<RenderItem> queue(3);
    queue[0].sortKey = {1, 1, 1};
    queue[1].sortKey = {1, 1, 2}; // Only material changed
    queue[2].sortKey = {1, 2, 2}; // Texture changed, material same
    
    deduplicator.ProcessQueue(queue, getBindShader(), getBindTexture(), getBindMaterial(), getDrawCallback());
    
    EXPECT_EQ(shaderBinds.size(), 1); // Only initial bind
    EXPECT_EQ(textureBinds.size(), 2); // Init + change at idx 2
    EXPECT_EQ(materialBinds.size(), 2); // Init + change at idx 1
    
    const auto& stats = deduplicator.GetStats();
    EXPECT_EQ(stats.actualShaderSwitches, 1);
    EXPECT_EQ(stats.actualTextureSwitches, 2);
    EXPECT_EQ(stats.actualMaterialSwitches, 2);
    EXPECT_EQ(stats.savedShaderSwitches(), 2);
    EXPECT_EQ(stats.savedTextureSwitches(), 1);
    EXPECT_EQ(stats.savedMaterialSwitches(), 1);
}

TEST_F(RenderStateDeduplicatorTest, LargeScaleRedundancyReductionOver80Percent)
{
    // Generate a large batch typical for a frame
    // 1000 items, but only 2 shaders, 5 textures, and 5 materials
    // Highly likely to have many duplicates when sorted
    const size_t numItems = 1000;
    auto queue = GenerateSortedQueue(numItems, 2, 5, 5);
    
    deduplicator.ProcessQueue(queue, getBindShader(), getBindTexture(), getBindMaterial(), getDrawCallback());
    
    const auto& stats = deduplicator.GetStats();
    
    EXPECT_EQ(stats.totalItems, numItems);
    EXPECT_EQ(drawnItems.size(), numItems);
    
    // We expect significant savings
    double reduction = stats.getReductionPercentage();
    std::cout << "Reduction percentage: " << reduction << "%" << std::endl;
    
    // Test verifies reduction is greater than 80% as requested
    EXPECT_GT(reduction, 80.0);
    
    // Detailed checks
    EXPECT_LE(stats.actualShaderSwitches, 5); // At most 5 shaders
    
    // Check that binds match actual calls
    EXPECT_EQ(shaderBinds.size(), stats.actualShaderSwitches);
    EXPECT_EQ(textureBinds.size(), stats.actualTextureSwitches);
    EXPECT_EQ(materialBinds.size(), stats.actualMaterialSwitches);
}

TEST_F(RenderStateDeduplicatorTest, MultipleFramesAccumulateStats)
{
    std::vector<RenderItem> queue1(5);
    for(auto& item : queue1) item.sortKey = {1, 1, 1};
    
    std::vector<RenderItem> queue2(5);
    for(auto& item : queue2) item.sortKey = {2, 2, 2};
    
    deduplicator.ProcessQueue(queue1, getBindShader(), getBindTexture(), getBindMaterial(), getDrawCallback());
    deduplicator.ProcessQueue(queue2, getBindShader(), getBindTexture(), getBindMaterial(), getDrawCallback());
    
    const auto& stats = deduplicator.GetStats();
    EXPECT_EQ(stats.totalItems, 10);
    
    // Frame 1: 1 shader switch (initial), Frame 2: 1 shader switch (initial relative to its queue start)
    EXPECT_EQ(stats.actualShaderSwitches, 2); 
    EXPECT_EQ(stats.actualTextureSwitches, 2);
    EXPECT_EQ(stats.actualMaterialSwitches, 2);
    
    EXPECT_EQ(stats.savedShaderSwitches(), 8);
    EXPECT_EQ(stats.savedTextureSwitches(), 8);
    EXPECT_EQ(stats.savedMaterialSwitches(), 8);
    
    EXPECT_DOUBLE_EQ(stats.getReductionPercentage(), 80.0); // 24 / 30 * 100
}

TEST_F(RenderStateDeduplicatorTest, ResetClearsStats)
{
    std::vector<RenderItem> queue(5);
    for(auto& item : queue) item.sortKey = {1, 1, 1};
    
    deduplicator.ProcessQueue(queue, getBindShader(), getBindTexture(), getBindMaterial(), getDrawCallback());
    
    EXPECT_GT(deduplicator.GetStats().totalItems, 0);
    
    deduplicator.ResetStats();
    
    const auto& stats = deduplicator.GetStats();
    EXPECT_EQ(stats.totalItems, 0);
    EXPECT_EQ(stats.actualShaderSwitches, 0);
    EXPECT_EQ(stats.savedShaderSwitches(), 0);
    EXPECT_DOUBLE_EQ(stats.getReductionPercentage(), 0.0);
}

TEST_F(RenderStateDeduplicatorTest, VerifySortKeyEquality)
{
    SortKey key1{1, 2, 3};
    SortKey key2{1, 2, 3};
    SortKey key3{1, 2, 4};
    SortKey key4{1, 5, 3};
    SortKey key5{6, 2, 3};

    EXPECT_TRUE(key1 == key2);
    EXPECT_FALSE(key1 == key3);
    EXPECT_FALSE(key1 == key4);
    EXPECT_FALSE(key1 == key5);
}

TEST_F(RenderStateDeduplicatorTest, VerifyDrawOrder)
{
    std::vector<RenderItem> queue(3);
    queue[0].sortKey = {1, 1, 1};
    queue[0].commandData = reinterpret_cast<void*>(10);
    
    queue[1].sortKey = {1, 1, 2};
    queue[1].commandData = reinterpret_cast<void*>(20);
    
    queue[2].sortKey = {1, 2, 2};
    queue[2].commandData = reinterpret_cast<void*>(30);
    
    deduplicator.ProcessQueue(queue, getBindShader(), getBindTexture(), getBindMaterial(), getDrawCallback());
    
    ASSERT_EQ(drawnItems.size(), 3);
    EXPECT_EQ(drawnItems[0].commandData, reinterpret_cast<void*>(10));
    EXPECT_EQ(drawnItems[1].commandData, reinterpret_cast<void*>(20));
    EXPECT_EQ(drawnItems[2].commandData, reinterpret_cast<void*>(30));
}

