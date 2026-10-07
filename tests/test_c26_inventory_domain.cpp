#include <gtest/gtest.h>
#include "../src/Client/Gameplay/InventoryDomain.h"

using namespace Client::Gameplay;
using namespace EterBase;

class InventoryDomainTest : public ::testing::Test {
protected:
    InventoryDomain domain;
};

TEST_F(InventoryDomainTest, TestSetItem_1x1)
{
    ItemData item{ ItemVnum(10), 1, {1, 1} };
    auto result = domain.SetItem(INVENTORY, ItemSlot(0), item);
    EXPECT_TRUE(result.has_value());
    
    auto retrieved = domain.GetItem(INVENTORY, ItemSlot(0));
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved.value().vnum.get(), 10);
    EXPECT_EQ(retrieved.value().count, 1);
}

TEST_F(InventoryDomainTest, TestSetItem_1x3_AntiOverflow)
{
    ItemData item{ ItemVnum(299), 1, {1, 3} };
    
    // Test normal placement
    auto result = domain.SetItem(INVENTORY, ItemSlot(0), item);
    EXPECT_TRUE(result.has_value());
    
    // Test overlap (placing 1x1 on the middle of 1x3)
    ItemData overlapItem{ ItemVnum(10), 1, {1, 1} };
    auto overlapResult = domain.SetItem(INVENTORY, ItemSlot(5), overlapItem);
    EXPECT_FALSE(overlapResult.has_value());
    EXPECT_EQ(overlapResult.error(), InventoryError::SlotOccupied);
    
    // Test bounds (placing 1x3 at the bottom of the page)
    auto outOfBoundsResult = domain.SetItem(INVENTORY, ItemSlot(40), item);
    EXPECT_FALSE(outOfBoundsResult.has_value());
    EXPECT_EQ(outOfBoundsResult.error(), InventoryError::SlotOutOfRange);
}

TEST_F(InventoryDomainTest, TestSwapItem)
{
    ItemData item1{ ItemVnum(10), 1, {1, 1} };
    ItemData item2{ ItemVnum(20), 1, {1, 2} };
    
    EXPECT_TRUE(domain.SetItem(INVENTORY, ItemSlot(0), item1).has_value());
    EXPECT_TRUE(domain.SetItem(INVENTORY, ItemSlot(1), item2).has_value());
    
    // Swap 1x1 with 1x2 (valid because slot 0 has space for 1x2 since slot 0 and 5 are free when item1 is removed, wait, slot 5 is free)
    auto swapResult = domain.SwapItem(INVENTORY, ItemSlot(0), INVENTORY, ItemSlot(1));
    EXPECT_TRUE(swapResult.has_value());
    
    auto retrieved1 = domain.GetItem(INVENTORY, ItemSlot(1));
    EXPECT_EQ(retrieved1.value().vnum.get(), 10);
    
    auto retrieved2 = domain.GetItem(INVENTORY, ItemSlot(0));
    EXPECT_EQ(retrieved2.value().vnum.get(), 20);
}

TEST_F(InventoryDomainTest, TestSwapItem_InvalidSize)
{
    ItemData item1{ ItemVnum(10), 1, {1, 1} }; // At slot 0
    ItemData blockItem{ ItemVnum(30), 1, {1, 1} }; // At slot 6
    ItemData item2{ ItemVnum(20), 1, {1, 2} }; // At slot 2
    
    EXPECT_TRUE(domain.SetItem(INVENTORY, ItemSlot(0), item1).has_value());
    EXPECT_TRUE(domain.SetItem(INVENTORY, ItemSlot(6), blockItem).has_value());
    EXPECT_TRUE(domain.SetItem(INVENTORY, ItemSlot(2), item2).has_value());
    
    // Swap item2 (1x2 at slot 2) with item1 (1x1 at slot 1, wait it's at slot 0)
    // If we swap item2 to slot 0, it needs slot 0 and 5. Slot 5 is free.
    // If we swap item2 to slot 1, it needs slot 1 and 6. Slot 6 is occupied!
    ItemData item3{ ItemVnum(40), 1, {1, 1} };
    EXPECT_TRUE(domain.SetItem(INVENTORY, ItemSlot(1), item3).has_value());
    
    auto swapResult = domain.SwapItem(INVENTORY, ItemSlot(2), INVENTORY, ItemSlot(1));
    EXPECT_FALSE(swapResult.has_value());
    EXPECT_EQ(swapResult.error(), InventoryError::SlotOccupied);
}

TEST_F(InventoryDomainTest, TestSplitItem)
{
    ItemData potions{ ItemVnum(27001), 200, {1, 1} };
    EXPECT_TRUE(domain.SetItem(INVENTORY, ItemSlot(0), potions).has_value());
    
    auto splitResult = domain.SplitItem(INVENTORY, ItemSlot(0), ItemSlot(1), 50);
    EXPECT_TRUE(splitResult.has_value());
    
    auto sourcePotions = domain.GetItem(INVENTORY, ItemSlot(0));
    EXPECT_EQ(sourcePotions.value().count, 150);
    
    auto newPotions = domain.GetItem(INVENTORY, ItemSlot(1));
    EXPECT_EQ(newPotions.value().count, 50);
}

TEST_F(InventoryDomainTest, TestSplitItem_InsufficientCount)
{
    ItemData potions{ ItemVnum(27001), 10, {1, 1} };
    EXPECT_TRUE(domain.SetItem(INVENTORY, ItemSlot(0), potions).has_value());
    
    auto splitResult = domain.SplitItem(INVENTORY, ItemSlot(0), ItemSlot(1), 10);
    EXPECT_FALSE(splitResult.has_value());
    EXPECT_EQ(splitResult.error(), InventoryError::InsufficientCount);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST_F(InventoryDomainTest, TestEquipmentOverwritePrevention)
{
    ItemData item{ ItemVnum(100), 1, {1, 3} }; // A 1x3 weapon
    
    // Equip the weapon
    auto result = domain.SetItem(EQUIPMENT, ItemSlot(0), item);
    EXPECT_TRUE(result.has_value());
    
    // Equip something else in the "overlapped" slot, which shouldn't happen for equipment
    ItemData otherItem{ ItemVnum(200), 1, {1, 1} };
    auto result2 = domain.SetItem(EQUIPMENT, ItemSlot(5), otherItem); // slot + 5
    EXPECT_TRUE(result2.has_value());
}

TEST_F(InventoryDomainTest, TestInvalidWindowType)
{
    ItemData item{ ItemVnum(10), 1, {1, 1} };
    auto result = domain.SetItem(RESERVED_WINDOW, ItemSlot(0), item);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), InventoryError::SlotOutOfRange);
}
