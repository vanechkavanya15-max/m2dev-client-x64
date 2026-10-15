#include "../src/Client/Network/DungeonCommandEncoder.h"
#include <iostream>
#include <cassert>

using namespace Client::Network;

int main() {
    std::cout << "Running test_c26_dungeon_command_encoder...\n";

    // 1. Enter Ymir 1B
    auto ymirEnter = DungeonCommandEncoder::EncodeEnter(101, true);
    assert(ymirEnter.has_value());
    assert(ymirEnter.value().size() == 6);

    // 2. Enter M2Dev 4B
    auto m2Enter = DungeonCommandEncoder::EncodeEnter(101, false);
    assert(m2Enter.has_value());
    assert(m2Enter.value().size() == 9);

    // 3. Ready Ymir
    auto ymirReady = DungeonCommandEncoder::EncodeReady(true);
    assert(ymirReady.has_value());
    assert(ymirReady.value().size() == 2);

    // 4. Leave M2Dev
    auto m2Leave = DungeonCommandEncoder::EncodeLeave(false);
    assert(m2Leave.has_value());
    assert(m2Leave.value().size() == 5);

    std::cout << "All DungeonCommandEncoder tests passed successfully.\n";
    return 0;
}
