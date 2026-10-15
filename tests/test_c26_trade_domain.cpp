#include <iostream>
#include <cassert>
#include "Client/Gameplay/TradeDomain.h"

using namespace Client::Gameplay;
using namespace EterBase;

void test_npc_shop() {
    NpcShop shop;

    // Register item
    ShopItem sword{ItemVnum(10), 1, Price(1000), Price(500)};
    shop.RegisterItem(sword);

    // Verify getting item
    auto item_opt = shop.GetItem(ItemVnum(10));
    assert(item_opt.has_value());
    assert(item_opt->buy_price.get() == 1000);
    
    // Verify buy
    auto buy_res = shop.BuyItem(ItemVnum(10), 2, Gold(5000));
    assert(buy_res.has_value());
    assert(buy_res.value().get() == 3000); // 5000 - (1000 * 2)

    // Verify buy with insufficient gold
    auto buy_fail = shop.BuyItem(ItemVnum(10), 1, Gold(500));
    assert(!buy_fail.has_value());

    // Verify sell
    auto sell_res = shop.SellItem(ItemVnum(10), 3);
    assert(sell_res.has_value());
    assert(sell_res.value().get() == 1500); // 500 * 3
}

void test_player_exchange() {
    EntityId p1(100);
    EntityId p2(200);

    PlayerExchange exchange(p1, p2);
    assert(exchange.GetState() == ExchangeState::Start);

    // Add item
    auto res_add_item = exchange.AddItem(p1, ItemSlot(0), ItemVnum(10));
    assert(res_add_item.has_value());
    assert(exchange.GetState() == ExchangeState::AddItem);

    // Add gold
    auto res_add_gold = exchange.AddGold(p2, Gold(5000));
    assert(res_add_gold.has_value());
    assert(exchange.GetState() == ExchangeState::AddGold);

    // Lock p1
    auto res_lock_p1 = exchange.Lock(p1);
    assert(res_lock_p1.has_value());
    assert(exchange.GetState() == ExchangeState::Lock);
    
    // Cannot add item after lock
    auto res_add_item_fail = exchange.AddItem(p1, ItemSlot(1), ItemVnum(20));
    assert(!res_add_item_fail.has_value());

    // Accept p1
    auto res_accept_p1 = exchange.Accept(p1);
    assert(res_accept_p1.has_value());

    // Cannot accept p2 without lock
    auto res_accept_p2_fail = exchange.Accept(p2);
    assert(!res_accept_p2_fail.has_value());

    // Lock p2
    exchange.Lock(p2);
    
    // Accept p2
    auto res_accept_p2 = exchange.Accept(p2);
    assert(res_accept_p2.has_value());

    // Verify both accepted
    assert(exchange.GetState() == ExchangeState::Accept);
    
    // Cannot cancel after accept
    auto res_cancel = exchange.Cancel();
    assert(!res_cancel.has_value());
}

void test_safebox() {
    SafeBox safebox(45); // 45 slots
    
    SafeBoxItem item{ItemVnum(50), 10};
    
    // Set item
    auto res_set = safebox.SetItem(ItemSlot(0), item);
    assert(res_set.has_value());
    
    // Get item
    auto item_opt = safebox.GetItem(ItemSlot(0));
    assert(item_opt.has_value());
    assert(item_opt->vnum.get() == 50);
    assert(item_opt->count == 10);
    
    // Set item out of bounds
    auto res_set_fail = safebox.SetItem(ItemSlot(50), item);
    assert(!res_set_fail.has_value());
    
    // Remove item
    auto res_remove = safebox.RemoveItem(ItemSlot(0));
    assert(res_remove.has_value());
    
    // Verify removal
    auto item_opt_empty = safebox.GetItem(ItemSlot(0));
    assert(!item_opt_empty.has_value());
}

int main() {
    test_npc_shop();
    test_player_exchange();
    test_safebox();
    
    std::cout << "All TradeDomain unit tests passed successfully!" << std::endl;
    return 0;
}
