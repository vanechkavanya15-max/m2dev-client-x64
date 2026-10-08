#include "Client/Gameplay/ExchangeStateGuard.h"
#include "Client/Gameplay/TradeDomain.h"
#include "EterBase/LogModern.h"

using namespace Client::Gameplay;
using namespace Client::Core;
using namespace EterBase;

int main() {
    EntityId p1(100);
    EntityId p2(200);

    PlayerExchange exchange(p1, p2);
    ExchangeStateGuard guard;

    // Test 1: No locks, should be valid
    auto result1 = guard.Validate(exchange, p1, p2);
    if (!result1.has_value()) {
        ModernLogger::Error("Test 1 Failed: Expected valid, got error.");
        return 1;
    }
    ModernLogger::Info("Test 1 Passed: Unlocked exchange validated.");

    // Test 2: Invalid participants
    auto result_invalid = guard.Validate(exchange, EntityId(999), p2);
    if (result_invalid.has_value() || result_invalid.error() != CommandError::InvalidParameter) {
        ModernLogger::Error("Test 2 Failed: Expected InvalidParameter error.");
        return 1;
    }
    ModernLogger::Info("Test 2 Passed: Invalid participant handled.");

    // Test 3: Lock initiator, should fail validation
    exchange.Lock(p1);
    auto result2 = guard.Validate(exchange, p1, p2);
    if (result2.has_value() || result2.error() != CommandError::InvalidParameter) {
        ModernLogger::Error("Test 3 Failed: Expected InvalidParameter after p1 locked.");
        return 1;
    }
    ModernLogger::Info("Test 3 Passed: p1 lock validated.");

    // Reset and test p2 lock
    PlayerExchange exchange_p2(p1, p2);
    exchange_p2.Lock(p2);
    auto result3 = guard.Validate(exchange_p2, p1, p2);
    if (result3.has_value() || result3.error() != CommandError::InvalidParameter) {
        ModernLogger::Error("Test 4 Failed: Expected InvalidParameter after p2 locked.");
        return 1;
    }
    ModernLogger::Info("Test 4 Passed: p2 lock validated.");

    ModernLogger::Info("All tests passed.");
    return 0;
}
