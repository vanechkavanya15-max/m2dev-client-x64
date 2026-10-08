#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <functional>
#include <string>

#include "../src/Client/Network/Handlers/ItemGroundAddHandler.h"

using namespace Client::Network::Handlers;

namespace Client::Core {}

void TestProcessItemGroundAdd_Success() {
    std::cout << "Running TestProcessItemGroundAdd_Success...\n";
    
    bool eventFired = false;
    
    uint32_t subId = UserInterface::Core::EventBus::GetInstance().Subscribe<ItemGroundAddEvent>(
        [&](const ItemGroundAddEvent& ev) {
            eventFired = true;
            assert(ev.dropVid.value() == 12345);
            assert(ev.itemVnum.value() == 50200);
            assert(ev.coords.x == 1000.0f);
            assert(ev.coords.y == 200000.0f); // 2000 * 100 logic scaling from legacy
            assert(ev.coords.z == 50.0f);
            assert(ev.ownershipLabel == "");
        }
    );

    PacketItemGroundAdd packet{};
    packet.header = 42;
    packet.length = sizeof(PacketItemGroundAdd);
    packet.lX = 1000;
    packet.lY = 2000;
    packet.lZ = 50;
    packet.dwVID = 12345;
    packet.dwVnum = 50200;

    std::span<const uint8_t> buffer(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));

    auto result = ProcessItemGroundAdd(buffer);
    
    assert(result.has_value());
    assert(eventFired);

    UserInterface::Core::EventBus::GetInstance().Unsubscribe<ItemGroundAddEvent>(subId);

    std::cout << "TestProcessItemGroundAdd_Success passed.\n";
}

void TestProcessItemGroundAdd_BufferUnderflow() {
    std::cout << "Running TestProcessItemGroundAdd_BufferUnderflow...\n";
    
    bool eventFired = false;
    uint32_t subId = UserInterface::Core::EventBus::GetInstance().Subscribe<ItemGroundAddEvent>(
        [&]([[maybe_unused]] const ItemGroundAddEvent& ev) {
            eventFired = true;
        }
    );

    std::vector<uint8_t> smallBuffer(sizeof(PacketItemGroundAdd) - 1, 0);

    auto result = ProcessItemGroundAdd(smallBuffer);
    
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::BufferUnderflow);
    assert(!eventFired);
    
    UserInterface::Core::EventBus::GetInstance().Unsubscribe<ItemGroundAddEvent>(subId);

    std::cout << "TestProcessItemGroundAdd_BufferUnderflow passed.\n";
}

int main() {
    TestProcessItemGroundAdd_Success();
    TestProcessItemGroundAdd_BufferUnderflow();
    std::cout << "All test_c26_item_ground_add_handler passed successfully.\n";
    return 0;
}
