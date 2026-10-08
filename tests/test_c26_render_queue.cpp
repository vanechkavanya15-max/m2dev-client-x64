#include <gtest/gtest.h>
#include "EterLib/Render/RenderQueue.h"

using namespace EterLib::Render;

// Helper to construct a RenderSortKey simulating different passes.
// We'll put Pass enum in the highest bits.
constexpr RenderSortKey MakeSortKey(Pass pass, uint32_t depth, uint32_t material)
{
    uint64_t key = 0;
    key |= (static_cast<uint64_t>(pass) & 0xFF) << 56;
    key |= (static_cast<uint64_t>(depth) & 0xFFFFFF) << 32;
    key |= (static_cast<uint64_t>(material) & 0xFFFFFFFF);
    return RenderSortKey{key};
}

class RenderQueueTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(RenderQueueTest, InitialStateIsEmpty)
{
    RenderQueue queue;
    EXPECT_EQ(queue.GetEntryCount(), 0);
    EXPECT_TRUE(queue.GetEntries().empty());
}

TEST_F(RenderQueueTest, SubmitIncreasesCountAndStoresData)
{
    RenderQueue queue;
    void* dummyCmd1 = reinterpret_cast<void*>(0x1234);
    void* dummyCmd2 = reinterpret_cast<void*>(0x5678);

    queue.Submit(MakeSortKey(Pass::Opaque, 10, 5), dummyCmd1, CommandType::Draw);
    queue.Submit(MakeSortKey(Pass::Transparent, 20, 1), dummyCmd2, CommandType::StateChange);

    EXPECT_EQ(queue.GetEntryCount(), 2);
    
    auto entries = queue.GetEntries();
    EXPECT_EQ(entries.size(), 2);
    
    EXPECT_EQ(entries[0].commandPtr, dummyCmd1);
    EXPECT_EQ(entries[0].type, CommandType::Draw);
    
    EXPECT_EQ(entries[1].commandPtr, dummyCmd2);
    EXPECT_EQ(entries[1].type, CommandType::StateChange);
}

TEST_F(RenderQueueTest, ClearRemovesAllEntries)
{
    RenderQueue queue;
    void* dummyCmd = reinterpret_cast<void*>(0xDEADBEEF);
    
    queue.Submit(MakeSortKey(Pass::UI, 0, 0), dummyCmd, CommandType::Draw);
    EXPECT_EQ(queue.GetEntryCount(), 1);
    
    queue.Clear();
    EXPECT_EQ(queue.GetEntryCount(), 0);
    EXPECT_TRUE(queue.GetEntries().empty());
}

TEST_F(RenderQueueTest, SortOrdersByPassCorrectly)
{
    RenderQueue queue;
    
    void* cmdOpaque = reinterpret_cast<void*>(1);
    void* cmdUI = reinterpret_cast<void*>(2);
    void* cmdDepthPrepass = reinterpret_cast<void*>(3);
    void* cmdAlphaTest = reinterpret_cast<void*>(4);
    void* cmdTransparent = reinterpret_cast<void*>(5);
    void* cmdAdditive = reinterpret_cast<void*>(6);
    
    // Submit in random order
    queue.Submit(MakeSortKey(Pass::AlphaTest, 1, 1), cmdAlphaTest, CommandType::Draw);
    queue.Submit(MakeSortKey(Pass::Additive, 1, 1), cmdAdditive, CommandType::Draw);
    queue.Submit(MakeSortKey(Pass::UI, 1, 1), cmdUI, CommandType::Draw);
    queue.Submit(MakeSortKey(Pass::Opaque, 1, 1), cmdOpaque, CommandType::Draw);
    queue.Submit(MakeSortKey(Pass::Transparent, 1, 1), cmdTransparent, CommandType::Draw);
    queue.Submit(MakeSortKey(Pass::DepthPrepass, 1, 1), cmdDepthPrepass, CommandType::Draw);
    
    queue.Sort();
    
    auto entries = queue.GetEntries();
    ASSERT_EQ(entries.size(), 6);
    
    // Expected order based on Pass enum value
    EXPECT_EQ(entries[0].commandPtr, cmdDepthPrepass);
    EXPECT_EQ(entries[1].commandPtr, cmdOpaque);
    EXPECT_EQ(entries[2].commandPtr, cmdAlphaTest);
    EXPECT_EQ(entries[3].commandPtr, cmdTransparent);
    EXPECT_EQ(entries[4].commandPtr, cmdAdditive);
    EXPECT_EQ(entries[5].commandPtr, cmdUI);
}

TEST_F(RenderQueueTest, SortOrdersWithinPassesByDepthAndMaterial)
{
    RenderQueue queue;
    
    void* cmdOpaqueFar = reinterpret_cast<void*>(10);
    void* cmdOpaqueNearDiffMat = reinterpret_cast<void*>(11);
    void* cmdOpaqueNear = reinterpret_cast<void*>(12);
    
    // Submitting with Opaque pass, different depths and materials
    queue.Submit(MakeSortKey(Pass::Opaque, 100, 1), cmdOpaqueFar, CommandType::Draw);
    queue.Submit(MakeSortKey(Pass::Opaque, 50, 2), cmdOpaqueNearDiffMat, CommandType::Draw);
    queue.Submit(MakeSortKey(Pass::Opaque, 50, 1), cmdOpaqueNear, CommandType::Draw);
    
    queue.Sort();
    
    auto entries = queue.GetEntries();
    ASSERT_EQ(entries.size(), 3);
    
    // Depth 50, Mat 1 should be first
    EXPECT_EQ(entries[0].commandPtr, cmdOpaqueNear);
    // Depth 50, Mat 2 should be second
    EXPECT_EQ(entries[1].commandPtr, cmdOpaqueNearDiffMat);
    // Depth 100, Mat 1 should be third
    EXPECT_EQ(entries[2].commandPtr, cmdOpaqueFar);
}

