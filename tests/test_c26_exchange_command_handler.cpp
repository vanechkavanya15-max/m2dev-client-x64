#include <iostream>
#include <cassert>
#include "Client/Gameplay/ExchangeCommandHandler.h"

using namespace Client::Gameplay;
using namespace Client::Core;
using namespace EterBase;

void TestExchangeLifecycle() {
    ExchangeCommandHandler handler;
    EntityId p1(100);
    EntityId p2(200);

    // 1. Uninitialized AddItem should fail
    auto res1 = handler.AddItem(p1, EterBase::ItemSlot(0), EterBase::ItemVnum(10));
    assert(!res1.has_value());
    assert(res1.error() == CommandError::InvalidParameter);

    // 2. Open exchange
    auto openRes = handler.Open(p1, p2);
    assert(openRes.has_value());
    assert(handler.IsActive());

    // 3. Open again should fail
    auto openRes2 = handler.Open(p1, p2);
    assert(!openRes2.has_value());

    // 4. AddItem success
    auto addRes = handler.AddItem(p1, EterBase::ItemSlot(1), EterBase::ItemVnum(500));
    assert(addRes.has_value());

    // 5. AddGold success
    auto goldRes = handler.AddGold(p2, Gold(1000));
    assert(goldRes.has_value());

    // 6. Lock
    auto lock1 = handler.Lock(p1);
    assert(lock1.has_value());
    auto lock2 = handler.Lock(p2);
    assert(lock2.has_value());

    // 7. Try AddItem after lock (should fail)
    auto addResFail = handler.AddItem(p1, EterBase::ItemSlot(2), EterBase::ItemVnum(600));
    assert(!addResFail.has_value());

    // 8. Accept
    auto acc1 = handler.Accept(p1);
    assert(acc1.has_value());
    auto acc2 = handler.Accept(p2);
    assert(acc2.has_value());

    // Verify state
    // handler should be inactive after both accepted
    assert(!handler.IsActive());
    assert(handler.GetExchange() == nullptr);
    
    std::cout << "TestExchangeLifecycle passed!\n";
}

void TestCancel() {
    ExchangeCommandHandler handler;
    EntityId p1(101);
    EntityId p2(202);

    handler.Open(p1, p2);
    handler.AddItem(p1, EterBase::ItemSlot(0), EterBase::ItemVnum(10));
    
    auto cancelRes = handler.Cancel();
    assert(cancelRes.has_value());
    
    // Exchange should be inactive after cancel
    assert(!handler.IsActive());
    
    // Cancelling an inactive exchange should fail
    auto cancelFail = handler.Cancel();
    assert(!cancelFail.has_value());
    
    std::cout << "TestCancel passed!\n";
}

void TestOpenValidation() {
    ExchangeCommandHandler handler;
    
    // Invalid IDs
    EntityId inv(0);
    EntityId p(5);
    
    auto res1 = handler.Open(inv, p);
    assert(!res1.has_value());
    
    auto res2 = handler.Open(p, inv);
    assert(!res2.has_value());
    
    // Same IDs
    auto res3 = handler.Open(p, p);
    assert(!res3.has_value());

    std::cout << "TestOpenValidation passed!\n";
}

int main() {
    TestExchangeLifecycle();
    TestCancel();
    TestOpenValidation();
    std::cout << "All ExchangeCommandHandler tests passed successfully!\n";
    return 0;
}
