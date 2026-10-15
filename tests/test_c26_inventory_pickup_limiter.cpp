#include <iostream>
#include <cassert>
#include "../src/Client/Gameplay/InventoryPickupLimiter.h"
#include "../src/Client/Core/StrongTypes.h"
#include "../src/Client/Core/DomainCommands.h"

using namespace Client::Gameplay;
using namespace Client::Core;

void test_successful_pickup() {
    InventoryPickupLimiter limiter;
    
    MapCoords playerPos{0.0f, 0.0f, 0.0f};
    MapCoords itemPos{100.0f, 0.0f, 0.0f}; // distance = 100
    
    auto result = limiter.CanPickup(playerPos, itemPos, 1000);
    assert(result.has_value());
    
    std::cout << "test_successful_pickup passed\n";
}

void test_rate_limited() {
    InventoryPickupLimiter limiter;
    
    MapCoords playerPos{0.0f, 0.0f, 0.0f};
    MapCoords itemPos{100.0f, 0.0f, 0.0f}; // distance = 100
    
    // First pickup is successful
    auto result = limiter.CanPickup(playerPos, itemPos, 1000);
    assert(result.has_value());
    
    // Second pickup within 100ms should fail with RateLimited
    auto result2 = limiter.CanPickup(playerPos, itemPos, 1050);
    assert(!result2.has_value());
    assert(result2.error() == CommandError::RateLimited);
    
    std::cout << "test_rate_limited passed\n";
}

void test_out_of_range() {
    InventoryPickupLimiter limiter;
    
    MapCoords playerPos{0.0f, 0.0f, 0.0f};
    MapCoords itemPos{400.0f, 0.0f, 0.0f}; // distance = 400
    
    auto result = limiter.CanPickup(playerPos, itemPos, 1000);
    assert(!result.has_value());
    assert(result.error() == CommandError::OutOfRange);
    
    std::cout << "test_out_of_range passed\n";
}

void test_pickup_after_cooldown() {
    InventoryPickupLimiter limiter;
    
    MapCoords playerPos{0.0f, 0.0f, 0.0f};
    MapCoords itemPos{100.0f, 0.0f, 0.0f}; // distance = 100
    
    // First pickup is successful
    auto result = limiter.CanPickup(playerPos, itemPos, 1000);
    assert(result.has_value());
    
    // Second pickup after 100ms should succeed
    auto result2 = limiter.CanPickup(playerPos, itemPos, 1150);
    assert(result2.has_value());
    
    std::cout << "test_pickup_after_cooldown passed\n";
}

int main() {
    test_successful_pickup();
    test_rate_limited();
    test_out_of_range();
    test_pickup_after_cooldown();
    
    std::cout << "All InventoryPickupLimiter tests passed!\n";
    return 0;
}
