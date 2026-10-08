#include <iostream>
#include <string>
#include <vector>
#include "../src/Client/Gameplay/SafeboxCommandHandler.h"
#include "../src/Client/Gameplay/InventoryDomain.h"
#include "../src/Client/Gameplay/TradeDomain.h"

int g_testsPassed = 0;
int g_testsFailed = 0;

void AssertEqual(bool condition, const std::string& testName) {
    if (condition) {
        std::cout << "[PASS] " << testName << std::endl;
        g_testsPassed++;
    } else {
        std::cout << "[FAIL] " << testName << std::endl;
        g_testsFailed++;
    }
}

using namespace Client::Gameplay;

void TestPasswordValidation() {
    InventoryDomain inventory;
    SafeBox safebox(135);
    SafeboxCommandHandler handler(inventory, safebox);

    auto resultEmpty = handler.ValidatePassword("");
    AssertEqual(!resultEmpty.has_value() && resultEmpty.error() == CommandError::InvalidPassword, "TestPasswordValidation: Empty password fails");

    auto resultSuccess = handler.ValidatePassword("my_password");
    AssertEqual(resultSuccess.has_value(), "TestPasswordValidation: Valid password opens safebox");
    AssertEqual(handler.IsOpen(), "TestPasswordValidation: IsOpen returns true after success");

    auto resultAlreadyOpened = handler.ValidatePassword("another");
    AssertEqual(!resultAlreadyOpened.has_value() && resultAlreadyOpened.error() == CommandError::AlreadyOpened, "TestPasswordValidation: Cannot validate when already opened");
}

void TestItemTransfer() {
    InventoryDomain inventory;
    SafeBox safebox(135);
    SafeboxCommandHandler handler(inventory, safebox);

    // Setup initial state
    EterBase::ItemSlot invSlot{0};
    EterBase::ItemSlot safeSlot{0};
    
    ItemData itemData{EterBase::ItemVnum{123}, 1, {1, 1}};
    auto setRes = inventory.SetItem(INVENTORY, invSlot, itemData);
    AssertEqual(setRes.has_value(), "TestItemTransfer: Setup inventory item");

    // Attempt transfer without opening
    auto failRes = handler.MoveItemToSafebox(invSlot, safeSlot);
    AssertEqual(!failRes.has_value() && failRes.error() == CommandError::NotOpened, "TestItemTransfer: Transfer fails if not opened");

    // Open safebox
    handler.ValidatePassword("123456");

    // Transfer Inv -> Safebox
    auto resInvToSafe = handler.MoveItemToSafebox(invSlot, safeSlot);
    AssertEqual(resInvToSafe.has_value(), "TestItemTransfer: Inv to Safebox success");
    AssertEqual(!inventory.GetItem(INVENTORY, invSlot).has_value(), "TestItemTransfer: Inv item removed");
    AssertEqual(safebox.GetItem(safeSlot).has_value() && safebox.GetItem(safeSlot)->vnum.get() == 123, "TestItemTransfer: Safebox item added");

    // Transfer Safebox -> Inv
    auto resSafeToInv = handler.MoveItemToInventory(safeSlot, invSlot);
    AssertEqual(resSafeToInv.has_value(), "TestItemTransfer: Safebox to Inv success");
    AssertEqual(!safebox.GetItem(safeSlot).has_value(), "TestItemTransfer: Safebox item removed");
    AssertEqual(inventory.GetItem(INVENTORY, invSlot).has_value() && inventory.GetItem(INVENTORY, invSlot)->vnum.get() == 123, "TestItemTransfer: Inv item added");
}

void TestCloseSafebox() {
    InventoryDomain inventory;
    SafeBox safebox(135);
    SafeboxCommandHandler handler(inventory, safebox);

    auto failRes = handler.Close();
    AssertEqual(!failRes.has_value() && failRes.error() == CommandError::NotOpened, "TestCloseSafebox: Fails if already closed");

    handler.ValidatePassword("pass");
    AssertEqual(handler.IsOpen(), "TestCloseSafebox: IsOpen is true");

    auto res = handler.Close();
    AssertEqual(res.has_value(), "TestCloseSafebox: Success");
    AssertEqual(!handler.IsOpen(), "TestCloseSafebox: IsOpen is false");
}

int main() {
    std::cout << "Running SafeboxCommandHandler Tests..." << std::endl;

    TestPasswordValidation();
    TestItemTransfer();
    TestCloseSafebox();

    std::cout << "\nTest Results: " << g_testsPassed << " passed, " << g_testsFailed << " failed." << std::endl;

    return g_testsFailed == 0 ? 0 : 1;
}
