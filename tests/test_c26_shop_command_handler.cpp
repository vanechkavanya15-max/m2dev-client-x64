#include <iostream>
#include <cassert>
#include <format>
#include <string_view>

// Mock the problematic windows.h locally for testing on linux
// We will exclude the #include <windows.h> block from LogModern.h by undefining _WIN32
#ifdef _WIN32
#undef _WIN32
#endif

#include "../src/Client/Gameplay/ShopCommandHandler.cpp"

using namespace Client::Gameplay;
using namespace Client::Core;

class MockNetworkPort : public INetworkPort {
public:
    bool connected = true;
    int sendCount = 0;

    [[nodiscard]] Result<void, PacketError> SendRaw(uint8_t opcode, std::span<const uint8_t> payload) override {
        sendCount++;
        return {};
    }

    [[nodiscard]] bool IsConnected() const noexcept override {
        return connected;
    }
};

void TestShopClosed() {
    auto port = std::make_shared<MockNetworkPort>();
    ShopCommandHandler handler(port);

    ShopBuyCommand buyCmd{0, 1};
    auto buyResult = handler.HandleBuy(buyCmd);
    assert(!buyResult.has_value());
    assert(buyResult.error() == CommandError::InvalidParameter);

    ShopSellCommand sellCmd{ItemSlot(0), 1};
    auto sellResult = handler.HandleSell(sellCmd);
    assert(!sellResult.has_value());
    assert(sellResult.error() == CommandError::InvalidParameter);

    assert(port->sendCount == 0);
    std::cout << "TestShopClosed passed.\n";
}

void TestBuyInvalidAmount() {
    auto port = std::make_shared<MockNetworkPort>();
    ShopCommandHandler handler(port);
    handler.OpenShop();

    ShopBuyCommand buyCmd{0, 0};
    auto buyResult = handler.HandleBuy(buyCmd);
    assert(!buyResult.has_value());
    assert(buyResult.error() == CommandError::InvalidParameter);

    assert(port->sendCount == 0);
    std::cout << "TestBuyInvalidAmount passed.\n";
}

void TestBuyInvalidSlot() {
    auto port = std::make_shared<MockNetworkPort>();
    ShopCommandHandler handler(port);
    handler.OpenShop();

    ShopBuyCommand buyCmd{40, 1}; // Max slot is 40 (0-39)
    auto buyResult = handler.HandleBuy(buyCmd);
    assert(!buyResult.has_value());
    assert(buyResult.error() == CommandError::InvalidParameter);

    assert(port->sendCount == 0);
    std::cout << "TestBuyInvalidSlot passed.\n";
}

void TestBuyValid() {
    auto port = std::make_shared<MockNetworkPort>();
    ShopCommandHandler handler(port);
    handler.OpenShop();

    ShopBuyCommand buyCmd{15, 1};
    auto buyResult = handler.HandleBuy(buyCmd);
    assert(buyResult.has_value());

    assert(port->sendCount == 1);
    std::cout << "TestBuyValid passed.\n";
}

void TestSellInvalidAmount() {
    auto port = std::make_shared<MockNetworkPort>();
    ShopCommandHandler handler(port);
    handler.OpenShop();

    ShopSellCommand sellCmd{ItemSlot(1), 0};
    auto sellResult = handler.HandleSell(sellCmd);
    assert(!sellResult.has_value());
    assert(sellResult.error() == CommandError::InvalidParameter);

    assert(port->sendCount == 0);
    std::cout << "TestSellInvalidAmount passed.\n";
}

void TestSellValid() {
    auto port = std::make_shared<MockNetworkPort>();
    ShopCommandHandler handler(port);
    handler.OpenShop();

    ShopSellCommand sellCmd{ItemSlot(1), 10};
    auto sellResult = handler.HandleSell(sellCmd);
    assert(sellResult.has_value());

    assert(port->sendCount == 1);
    std::cout << "TestSellValid passed.\n";
}

int main() {
    std::cout << "Running ShopCommandHandler tests...\n";
    TestShopClosed();
    TestBuyInvalidAmount();
    TestBuyInvalidSlot();
    TestBuyValid();
    TestSellInvalidAmount();
    TestSellValid();
    std::cout << "All ShopCommandHandler tests passed.\n";
    return 0;
}
