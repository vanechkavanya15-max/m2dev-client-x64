#include <iostream>
#include <cassert>
#include "Client/Gameplay/InventoryGridManager.h"

using namespace Client::Gameplay;

using namespace EterBase;

void test_initialization() {
    InventoryGridManager grid(5, 9, 2);
    
    // Test that the first slot is empty
    auto res = grid.CanPlaceItem(ItemSlot(0), 1);
    assert(res.has_value());
    
    // Find empty slot for 1x1 should return 0
    auto slotRes = grid.FindEmptySlot(1);
    assert(slotRes.has_value());
    assert(slotRes.value().get() == 0);
}

void test_1x1_placement() {
    InventoryGridManager grid(5, 9, 2);
    
    // Occupy 1x1 item at slot 0
    auto res = grid.OccupySlot(ItemSlot(0), 1);
    assert(res.has_value());
    
    // Cannot place 1x1 item at slot 0 again
    auto resFail = grid.CanPlaceItem(ItemSlot(0), 1);
    assert(!resFail.has_value());
    assert(resFail.error() == Client::Core::CommandError::InventoryFull);
    
    // Find empty slot should now be 1
    auto slotRes = grid.FindEmptySlot(1);
    assert(slotRes.has_value());
    assert(slotRes.value().get() == 1);
    
    // Free the slot
    auto freeRes = grid.FreeSlot(ItemSlot(0), 1);
    assert(freeRes.has_value());
    
    // Verify it's freed
    auto resSuccess = grid.CanPlaceItem(ItemSlot(0), 1);
    assert(resSuccess.has_value());
}

void test_1x2_placement() {
    InventoryGridManager grid(5, 9, 2);
    
    // Occupy 1x2 item at slot 0 (takes 0 and 5)
    auto res = grid.OccupySlot(ItemSlot(0), 2);
    assert(res.has_value());
    
    // Cannot place 1x1 item at slot 0 or 5
    auto resFail0 = grid.CanPlaceItem(ItemSlot(0), 1);
    assert(!resFail0.has_value());
    
    auto resFail5 = grid.CanPlaceItem(ItemSlot(5), 1);
    assert(!resFail5.has_value());
    
    // Can place at 1
    auto resSuccess = grid.CanPlaceItem(ItemSlot(1), 1);
    assert(resSuccess.has_value());
}

void test_1x3_placement() {
    InventoryGridManager grid(5, 9, 2);
    
    // Occupy 1x3 item at slot 0 (takes 0, 5, 10)
    auto res = grid.OccupySlot(ItemSlot(0), 3);
    assert(res.has_value());
    
    // Verify 0, 5, 10 are occupied
    assert(!grid.CanPlaceItem(ItemSlot(0), 1).has_value());
    assert(!grid.CanPlaceItem(ItemSlot(5), 1).has_value());
    assert(!grid.CanPlaceItem(ItemSlot(10), 1).has_value());
    
    // Verify 15 is free
    assert(grid.CanPlaceItem(ItemSlot(15), 1).has_value());
}

void test_out_of_bounds() {
    InventoryGridManager grid(5, 9, 2);
    
    // Page 0 goes from 0 to 44. Page 1 goes from 45 to 89.
    
    // Place 1x1 at 89 (last slot) -> OK
    auto resOk = grid.CanPlaceItem(ItemSlot(89), 1);
    assert(resOk.has_value());
    
    // Place 1x1 at 90 (out of bounds) -> Fail
    auto resFail = grid.CanPlaceItem(ItemSlot(90), 1);
    assert(!resFail.has_value());
    assert(resFail.error() == Client::Core::CommandError::OutOfRange);
    
    // Place 1x2 at 40 (bottom row of page 0, would cross to page 1) -> Fail
    auto resFailCross = grid.CanPlaceItem(ItemSlot(40), 2);
    assert(!resFailCross.has_value());
    assert(resFailCross.error() == Client::Core::CommandError::OutOfRange);
    
    // Place 1x2 at 85 (bottom row of page 1, would cross out of bounds) -> Fail
    auto resFailOut = grid.CanPlaceItem(ItemSlot(85), 2);
    assert(!resFailOut.has_value());
    assert(resFailOut.error() == Client::Core::CommandError::OutOfRange);
}

void test_find_empty_slot_full() {
    InventoryGridManager grid(1, 2, 1); // Very small grid: 1x2, 1 page (total 2 slots)
    
    auto res = grid.OccupySlot(ItemSlot(0), 2);
    assert(res.has_value());
    
    auto emptyRes = grid.FindEmptySlot(1);
    assert(!emptyRes.has_value());
    assert(emptyRes.error() == Client::Core::CommandError::InventoryFull);
}

int main() {
    test_initialization();
    test_1x1_placement();
    test_1x2_placement();
    test_1x3_placement();
    test_out_of_bounds();
    test_find_empty_slot_full();
    
    std::cout << "All InventoryGridManager unit tests passed successfully!" << std::endl;
    return 0;
}
