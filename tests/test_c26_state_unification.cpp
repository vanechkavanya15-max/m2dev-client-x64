#include "../src/Client/Core/WorldContext.h"
#include "../src/Client/Bridge/StranglerFacade.h"
#include <iostream>
#include <cassert>

using namespace Client::Core;
using namespace Client::Bridge;

void Test_StateUnification_Points() {
    WorldContext ctx;
    ctx.Reset();

    // Verify initial values
    assert(ctx.currentHp == 0);
    assert(ctx.maxHp == 0);
    assert(ctx.currentSp == 0);
    assert(ctx.maxSp == 0);
    assert(ctx.currentGold == 0);

    // Set HP (pointType 5)
    ctx.SetPoint(5, 12500);
    assert(ctx.GetPoint(5) == 12500);
    assert(ctx.currentHp == 12500);

    // Set Max HP (pointType 6)
    ctx.SetPoint(6, 15000);
    assert(ctx.GetPoint(6) == 15000);
    assert(ctx.maxHp == 15000);

    // Set SP (pointType 7)
    ctx.SetPoint(7, 3200);
    assert(ctx.GetPoint(7) == 3200);
    assert(ctx.currentSp == 3200);

    // Set Max SP (pointType 8)
    ctx.SetPoint(8, 4000);
    assert(ctx.GetPoint(8) == 4000);
    assert(ctx.maxSp == 4000);

    // Set Gold (pointType 11)
    ctx.SetPoint(11, 987654321);
    assert(ctx.GetPoint(11) == 987654321);
    assert(ctx.currentGold == 987654321);

    // Reset clears everything
    ctx.Reset();
    assert(ctx.GetPoint(5) == 0);
    assert(ctx.currentHp == 0);
    assert(ctx.currentGold == 0);

    std::cout << "[OK] Test_StateUnification_Points passed!\n";
}

void Test_StateUnification_StranglerFacade() {
    auto& facade = StranglerFacade::Instance();
    auto& ctx = facade.GetWorldContext();

    ctx.SetPoint(5, 7777);
    assert(facade.GetWorldContext().GetPoint(5) == 7777);
    assert(facade.GetWorldContext().currentHp == 7777);

    ctx.SetPoint(6, 9999);
    assert(facade.GetWorldContext().GetPoint(6) == 9999);
    assert(facade.GetWorldContext().maxHp == 9999);

    // Clean up
    ctx.Reset();
    assert(facade.GetWorldContext().GetPoint(5) == 0);

    std::cout << "[OK] Test_StateUnification_StranglerFacade passed!\n";
}

int main() {
    std::cout << "Running State Unification (B2) tests...\n";
    Test_StateUnification_Points();
    Test_StateUnification_StranglerFacade();
    std::cout << "All State Unification tests passed successfully!\n";
    return 0;
}
