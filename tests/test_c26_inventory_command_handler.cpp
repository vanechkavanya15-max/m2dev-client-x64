#include <cassert>
#include <iostream>
#include <format>
#include "../src/Client/Gameplay/InventoryCommandHandler.h"
#include "../src/Client/Gameplay/InventoryDomain.h"

using namespace Client::Gameplay;
using namespace EterBase;

class MockConditionProvider : public IPlayerConditionProvider {
public:
    bool isDead = false;
    bool isTrading = false;

    bool IsDead() const override { return isDead; }
    bool IsTrading() const override { return isTrading; }
};

void test_inventory_command_handler() {
    std::cout << "Starting test_inventory_command_handler..." << std::endl;
    
    constexpr uint8_t INVENTORY = static_cast<uint8_t>(InventoryWindow::Inventory);
    InventoryDomain domain;
    MockConditionProvider conditions;
    InventoryCommandHandler handler(domain, conditions);

    // Setup initial item
    ItemData item;
    item.vnum = ItemVnum(123);
    item.count = 1;
    item.size = {1, 1};
    
    auto setRes = domain.SetItem(INVENTORY, ItemSlot(0), item);
    assert(setRes.has_value());

    // 1. Test PlayerDead
    conditions.isDead = true;
    auto useRes = handler.Handle(UseItemCommand{INVENTORY, ItemSlot(0)});
    assert(!useRes.has_value());
    assert(useRes.error() == CommandError::PlayerDead);
    
    // 2. Test PlayerTrading
    conditions.isDead = false;
    conditions.isTrading = true;
    auto moveRes = handler.Handle(MoveItemCommand{INVENTORY, ItemSlot(0), INVENTORY, ItemSlot(1)});
    assert(!moveRes.has_value());
    assert(moveRes.error() == CommandError::PlayerTrading);

    // 3. Test SlotEmpty
    conditions.isTrading = false;
    auto dropRes = handler.Handle(DropItemCommand{INVENTORY, ItemSlot(1)}); // slot 1 is empty
    assert(!dropRes.has_value());
    assert(dropRes.error() == CommandError::SlotEmpty);

    // 4. Test Successful HandleUse
    auto okUse = handler.Handle(UseItemCommand{INVENTORY, ItemSlot(0)});
    assert(okUse.has_value());

    // 5. Test Successful HandleMove
    auto okMove = handler.Handle(MoveItemCommand{INVENTORY, ItemSlot(0), INVENTORY, ItemSlot(2)});
    assert(okMove.has_value());
    
    // Verify item was moved
    auto getItemRes0 = domain.GetItem(INVENTORY, ItemSlot(0));
    assert(!getItemRes0.has_value()); // should be empty now
    auto getItemRes2 = domain.GetItem(INVENTORY, ItemSlot(2));
    assert(getItemRes2.has_value()); // should exist

    // 6. Test Successful HandleDrop
    auto okDrop = handler.Handle(DropItemCommand{INVENTORY, ItemSlot(2)});
    assert(okDrop.has_value());
    
    // Verify item was removed
    auto getItemRes2AfterDrop = domain.GetItem(INVENTORY, ItemSlot(2));
    assert(!getItemRes2AfterDrop.has_value());

    // 7. Test invalid swap (e.g. out of bounds)
    item.size = {2, 2};
    domain.SetItem(INVENTORY, ItemSlot(0), item);
    auto badMove = handler.Handle(MoveItemCommand{INVENTORY, ItemSlot(0), INVENTORY, ItemSlot(4)}); // 4 is at edge, 2x2 won't fit
    assert(!badMove.has_value());
    assert(badMove.error() == CommandError::InvalidCommand);

    std::cout << "test_inventory_command_handler passed!" << std::endl;
}

int main() {
    test_inventory_command_handler();
    return 0;
}
