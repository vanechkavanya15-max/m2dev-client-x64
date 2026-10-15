#include <gtest/gtest.h>
#include "../src/EterLib/Render/ModernStateManagerBridge.h"
#include <string_view>

using namespace EterLib::Render;

class ModernStateManagerBridgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        ModernStateManagerBridge::Instance().Reset();
    }
};

TEST_F(ModernStateManagerBridgeTest, InitialStateIsZero) {
    auto& bridge = ModernStateManagerBridge::Instance();
    EXPECT_EQ(bridge.GetStateSwitchCount(), 0);
    EXPECT_EQ(bridge.GetActiveStateCount(), 0);
}

TEST_F(ModernStateManagerBridgeTest, StateBitmask256Operations) {
    StateBitmask256 bitmask;
    EXPECT_EQ(bitmask.Count(), 0);

    bitmask.Set(5);
    EXPECT_TRUE(bitmask.Test(5));
    EXPECT_FALSE(bitmask.Test(4));
    EXPECT_EQ(bitmask.Count(), 1);

    bitmask.Set(65);
    EXPECT_TRUE(bitmask.Test(65));
    EXPECT_EQ(bitmask.Count(), 2);

    bitmask.Clear(5);
    EXPECT_FALSE(bitmask.Test(5));
    EXPECT_EQ(bitmask.Count(), 1);

    bitmask.ClearAll();
    EXPECT_EQ(bitmask.Count(), 0);
}

TEST_F(ModernStateManagerBridgeTest, FixedStateStackOperations) {
    FixedStateStack<DWORD, 3> stack;
    EXPECT_TRUE(stack.IsEmpty());
    EXPECT_EQ(stack.Size(), 0);

    EXPECT_TRUE(stack.Push(10));
    EXPECT_TRUE(stack.Push(20));
    EXPECT_TRUE(stack.Push(30));
    EXPECT_FALSE(stack.Push(40)); // Overflow

    EXPECT_EQ(stack.Size(), 3);

    auto val = stack.Pop();
    EXPECT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), 30);

    stack.Pop();
    stack.Pop();

    auto val2 = stack.Pop();
    EXPECT_FALSE(val2.has_value()); // Underflow
    EXPECT_EQ(val2.error(), "Stack underflow");
}
