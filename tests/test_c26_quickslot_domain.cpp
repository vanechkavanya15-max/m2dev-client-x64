#include <cassert>
#include <iostream>
#include "../src/Client/Gameplay/QuickslotDomain.h"

using namespace Client::Gameplay;
using namespace EterBase;

int main()
{
    QuickslotDomain domain;

    // Test 1: Stan poczatkowy
    {
        assert(domain.GetPage() == 0);
        for (uint32_t i = 0; i < QuickslotDomain::QUICKSLOT_MAX_NUM; ++i)
        {
            auto slot = domain.GetSlot(i);
            assert(slot.has_value());
            assert(slot->IsEmpty());
        }
    }

    // Test 2: Ustawianie i pobieranie slotu globalnego
    {
        QuickslotItem item{1, 5}; // typ 1 (np. inventory), pozycja 5
        auto setRes = domain.SetSlot(0, item);
        assert(setRes.has_value());

        auto getRes = domain.GetSlot(0);
        assert(getRes.has_value());
        assert(getRes->type == 1);
        assert(getRes->position == 5);
        assert(!getRes->IsEmpty());
    }

    // Test 3: Stronicowanie i sloty lokalne
    {
        // Strona 0, lokalny slot 2 -> globalny 2
        QuickslotItem item2{2, 10}; // typ 2 (np. skill), pozycja 10
        assert(domain.SetLocalSlot(2, item2).has_value());
        assert(domain.GetSlot(2)->type == 2);

        // Zmiana strony na 1 -> lokalny slot 0 = globalny 8
        domain.SetPage(1);
        assert(domain.GetPage() == 1);
        assert(domain.LocalToGlobalIndex(0) == 8);

        QuickslotItem itemPage1{3, 7};
        assert(domain.SetLocalSlot(0, itemPage1).has_value());
        assert(domain.GetSlot(8)->type == 3);

        // Zawijanie stron
        domain.SetPage(4); // modulo QUICKSLOT_MAX_LINE (4) -> 0
        assert(domain.GetPage() == 0);

        domain.SetPage(-1); // 4 + (-1) -> 3
        assert(domain.GetPage() == 3);
        assert(domain.LocalToGlobalIndex(0) == 24);
    }

    // Test 4: Zamiana slotow (Swap)
    {
        domain.SetPage(0);
        QuickslotItem slotA{1, 100};
        QuickslotItem slotB{2, 200};
        assert(domain.SetSlot(5, slotA).has_value());
        assert(domain.SetSlot(6, slotB).has_value());

        assert(domain.SwapSlots(5, 6).has_value());
        assert(domain.GetSlot(5)->type == 2);
        assert(domain.GetSlot(5)->position == 200);
        assert(domain.GetSlot(6)->type == 1);
        assert(domain.GetSlot(6)->position == 100);
    }

    // Test 5: Czyszczenie slotu i Clear calkowity
    {
        assert(domain.ClearSlot(5).has_value());
        assert(domain.GetSlot(5)->IsEmpty());

        domain.Clear();
        assert(domain.GetPage() == 0);
        assert(domain.GetSlot(6)->IsEmpty());
    }

    // Test 6: Obsluga bledow (out of range)
    {
        assert(!domain.GetSlot(36).has_value());
        assert(domain.GetSlot(36).error() == InventoryError::SlotOutOfRange);

        QuickslotItem item{1, 1};
        assert(!domain.SetSlot(100, item).has_value());
        assert(!domain.SetLocalSlot(8, item).has_value()); // lokalny max to 7
        assert(!domain.SwapSlots(0, 50).has_value());
    }

    std::cout << "test_c26_quickslot_domain: ALL TESTS PASSED (100%)\n";
    return 0;
}
