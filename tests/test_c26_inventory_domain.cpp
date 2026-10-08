#include <cassert>
#include <iostream>
#include "../src/Client/Gameplay/InventoryDomain.h"

using namespace Client::Gameplay;
using namespace EterBase;

int main() {
    InventoryDomain domain;

    // Test 1: SetItem_1x1
    {
        ItemData item{ ItemVnum(10), 1, {1, 1} };
        auto result = domain.SetItem(InventoryWindow::Inventory, ItemSlot(0), item);
        assert(result.has_value());
        
        auto retrieved = domain.GetItem(InventoryWindow::Inventory, ItemSlot(0));
        assert(retrieved.has_value());
        assert(retrieved.value().vnum.get() == 10);
        assert(retrieved.value().count == 1);
    }

    // Test 2: SetItem_1x3_AntiOverflow
    {
        ItemData item{ ItemVnum(299), 1, {1, 3} };
        
        // Placement on slot 10
        auto result = domain.SetItem(InventoryWindow::Inventory, ItemSlot(10), item);
        assert(result.has_value());
        
        // Overlap test (placing 1x1 on the middle of 1x3, slot 10 + 5 = 15)
        ItemData overlapItem{ ItemVnum(10), 1, {1, 1} };
        auto overlapResult = domain.SetItem(InventoryWindow::Inventory, ItemSlot(15), overlapItem);
        assert(!overlapResult.has_value());
        assert(overlapResult.error() == InventoryError::SlotOccupied);
        
        // Out of bounds (slot 40 with 1x3 crosses bottom border of page 1)
        auto outOfBoundsResult = domain.SetItem(InventoryWindow::Inventory, ItemSlot(40), item);
        assert(!outOfBoundsResult.has_value());
        assert(outOfBoundsResult.error() == InventoryError::SlotOutOfRange);
    }

    // Test 3: SwapItem
    {
        ItemData item1{ ItemVnum(10), 1, {1, 1} };
        ItemData item2{ ItemVnum(20), 1, {1, 2} };
        
        assert(domain.SetItem(InventoryWindow::Inventory, ItemSlot(1), item1).has_value());
        assert(domain.SetItem(InventoryWindow::Inventory, ItemSlot(2), item2).has_value());
        
        auto swapResult = domain.SwapItem(InventoryWindow::Inventory, ItemSlot(1), InventoryWindow::Inventory, ItemSlot(2));
        assert(swapResult.has_value());
        
        auto retrieved1 = domain.GetItem(InventoryWindow::Inventory, ItemSlot(2));
        assert(retrieved1.value().vnum.get() == 10);
        
        auto retrieved2 = domain.GetItem(InventoryWindow::Inventory, ItemSlot(1));
        assert(retrieved2.value().vnum.get() == 20);
    }

    // Test 4: SplitItem
    {
        ItemData potions{ ItemVnum(27001), 200, {1, 1} };
        assert(domain.SetItem(InventoryWindow::Inventory, ItemSlot(3), potions).has_value());
        
        auto splitResult = domain.SplitItem(InventoryWindow::Inventory, ItemSlot(3), ItemSlot(4), 50);
        assert(splitResult.has_value());
        
        auto sourcePotions = domain.GetItem(InventoryWindow::Inventory, ItemSlot(3));
        assert(sourcePotions.value().count == 150);
        
        auto newPotions = domain.GetItem(InventoryWindow::Inventory, ItemSlot(4));
        assert(newPotions.value().count == 50);

        // Insufficient count split
        auto splitFail = domain.SplitItem(InventoryWindow::Inventory, ItemSlot(3), ItemSlot(5), 200);
        assert(!splitFail.has_value());
        assert(splitFail.error() == InventoryError::InsufficientCount);
    }

    // Test 5: FindEmptyCell
    {
        int32_t emptySlot = domain.FindEmptyCell();
        assert(emptySlot == 5); // slots 0, 1, 2, 3, 4, 10, 15 are occupied
        assert(domain.GetItem(InventoryWindow::Inventory, ItemSlot(emptySlot)).error() == InventoryError::SlotEmpty);

        // FindEmptyCell for size 1x2
        int32_t emptyFor2 = domain.FindEmptyCell(ItemSize{1, 2});
        assert(emptyFor2 >= 0);
    }

    // Test 6: Delegate callback
    {
        uint32_t callbackCounter = 0;
        domain.SetSlotUpdateCallback([&callbackCounter](const InventorySlotUpdatedEvent& ev) {
            callbackCounter++;
            assert(ev.windowType == InventoryWindow::Inventory);
        });

        ItemData testCbItem{ ItemVnum(999), 1, {1, 1} };
        assert(domain.SetItem(InventoryWindow::Inventory, ItemSlot(5), testCbItem).has_value());
        assert(callbackCounter == 1);

        assert(domain.RemoveItem(InventoryWindow::Inventory, ItemSlot(5)).has_value());
        assert(callbackCounter == 2);
    }

    // Test 7: ItemData z sockets i attributes
    {
        ItemData fullItem;
        fullItem.vnum = ItemVnum(11299);
        fullItem.count = 1;
        fullItem.size = {1, 2};
        fullItem.sockets[0] = 28441; // Kamien Witalnosci +4
        fullItem.sockets[1] = 28443; // Kamien Przyspieszenia +4
        fullItem.attributes[0] = {1, 2000};
        fullItem.attributes[1] = {23, 10};

        assert(domain.SetItem(InventoryWindow::Inventory, ItemSlot(20), fullItem).has_value());
        auto retrieved = domain.GetItem(InventoryWindow::Inventory, ItemSlot(20));
        assert(retrieved.has_value());
        assert(retrieved->sockets[0] == 28441);
        assert(retrieved->sockets[1] == 28443);
        assert(retrieved->attributes[0].type == 1);
        assert(retrieved->attributes[0].value == 2000);
    }

    // Test 8: EquipmentOverwritePrevention
    {
        ItemData eqWeapon{ ItemVnum(100), 1, {1, 3} };
        auto eqResult = domain.SetItem(InventoryWindow::Equipment, ItemSlot(0), eqWeapon);
        assert(eqResult.has_value());
        
        ItemData eqOther{ ItemVnum(200), 1, {1, 1} };
        auto eqResult2 = domain.SetItem(InventoryWindow::Equipment, ItemSlot(5), eqOther);
        assert(eqResult2.has_value());
    }

    // Test 9: InvalidWindowType
    {
        ItemData item{ ItemVnum(10), 1, {1, 1} };
        auto result = domain.SetItem(InventoryWindow::Reserved, ItemSlot(0), item);
        assert(!result.has_value());
        assert(result.error() == InventoryError::SlotOutOfRange);
    }

    // Test 10: Clear
    {
        domain.Clear();
        // After clear, slot 0 and slot 20 should be empty
        assert(!domain.GetItem(InventoryWindow::Inventory, ItemSlot(0)).has_value());
        assert(!domain.GetItem(InventoryWindow::Inventory, ItemSlot(20)).has_value());
        assert(!domain.GetItem(InventoryWindow::Equipment, ItemSlot(0)).has_value());
        assert(domain.FindEmptyCell() == 0);
    }

    std::cout << "test_c26_inventory_domain: ALL TESTS PASSED (100%)\n";
    return 0;
}
