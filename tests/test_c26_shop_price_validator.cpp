#include "../src/Client/Gameplay/ShopPriceValidator.h"
#include <cassert>
#include <iostream>

void TestShopPriceValidator() {
    Client::Core::WorldContext ctx;
    ctx.currentGold = 1000;
    ctx.currentCheque = 50;
    ctx.currentGaya = 100;
    
    Client::Gameplay::ShopPriceValidator validator(ctx);
    
    // Test 1: Succesful gold purchase
    auto res1 = validator.ValidatePurchase(Client::Gameplay::CurrencyType::Gold, 100, 2);
    assert(res1.has_value() && res1.value() == 800);
    
    // Test 2: Unsuccesful gold purchase
    auto res2 = validator.ValidatePurchase(Client::Gameplay::CurrencyType::Gold, 600, 2);
    assert(!res2.has_value() && res2.error() == Client::Core::CommandError::InvalidParameter);
    
    // Test 3: Overflow
    auto res3 = validator.ValidatePurchase(Client::Gameplay::CurrencyType::Gold, 9223372036854775807LL, 2);
    assert(!res3.has_value() && res3.error() == Client::Core::CommandError::InvalidParameter);
    
    // Test 4: Cheque success
    auto res4 = validator.ValidatePurchase(Client::Gameplay::CurrencyType::Cheque, 10, 3);
    assert(res4.has_value() && res4.value() == 20);
    
    // Test 5: Gaya fail
    auto res5 = validator.ValidatePurchase(Client::Gameplay::CurrencyType::Gaya, 60, 2);
    assert(!res5.has_value() && res5.error() == Client::Core::CommandError::InvalidParameter);
    
    // Test 6: Invalid param
    auto res6 = validator.ValidatePurchase(Client::Gameplay::CurrencyType::Gold, -10, 2);
    assert(!res6.has_value() && res6.error() == Client::Core::CommandError::InvalidParameter);
    
    std::cout << "All ShopPriceValidator tests passed!\n";
}

int main() {
    TestShopPriceValidator();
    return 0;
}
