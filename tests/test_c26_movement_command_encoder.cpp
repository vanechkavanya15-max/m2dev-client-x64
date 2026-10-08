// tests/test_c26_movement_command_encoder.cpp
#include "../src/Client/Network/MovementCommandEncoder.h"
#include "../src/UserInterface/Packet.h"
#include <cassert>
#include <iostream>

void TestEncodeBasic() {
    Client::Core::MoveCommand cmd;
    cmd.destination.x = 10.5f;
    cmd.destination.y = -20.5f;
    cmd.rotation = 45.0f;
    cmd.moveType = 0;
    cmd.clientTimestamp = 12345;

    auto result = Client::Network::MovementCommandEncoder::Encode(cmd);
    assert(result.has_value());
    assert(result.value().size() == sizeof(TPacketCGMove));

    const TPacketCGMove* packet = reinterpret_cast<const TPacketCGMove*>(result.value().data());
    
    assert(packet->header == CG::MOVE);
    assert(packet->length == sizeof(TPacketCGMove));
    assert(packet->bFunc == 1);
    assert(packet->bArg == 0);
    assert(packet->bRot == 9); // 45.0f / 5.0f
    assert(packet->lX == 1050);
    assert(packet->lY == -2050);
    assert(packet->dwTime == 12345);
}

void TestEncodeNegativeRotation() {
    Client::Core::MoveCommand cmd;
    cmd.destination.x = 0.0f;
    cmd.destination.y = 0.0f;
    cmd.rotation = -10.0f;
    cmd.moveType = 1;
    cmd.clientTimestamp = 54321;

    auto result = Client::Network::MovementCommandEncoder::Encode(cmd);
    assert(result.has_value());

    const TPacketCGMove* packet = reinterpret_cast<const TPacketCGMove*>(result.value().data());
    
    // -10.0f wraps to 350.0f
    // 350.0f / 5.0f = 70
    assert(packet->bRot == 70);
    assert(packet->bArg == 1);
}

void TestEncodeLargeRotation() {
    Client::Core::MoveCommand cmd;
    cmd.destination.x = 0.0f;
    cmd.destination.y = 0.0f;
    cmd.rotation = 730.0f; // 730 % 360 = 10
    cmd.moveType = 0;
    cmd.clientTimestamp = 0;

    auto result = Client::Network::MovementCommandEncoder::Encode(cmd);
    assert(result.has_value());

    const TPacketCGMove* packet = reinterpret_cast<const TPacketCGMove*>(result.value().data());
    
    assert(packet->bRot == 2); // 10.0f / 5.0f
}

void TestEncodeLargeNegativeRotation() {
    Client::Core::MoveCommand cmd;
    cmd.destination.x = 0.0f;
    cmd.destination.y = 0.0f;
    cmd.rotation = -730.0f; // -730 % 360 = -10 -> 350
    cmd.moveType = 0;
    cmd.clientTimestamp = 0;

    auto result = Client::Network::MovementCommandEncoder::Encode(cmd);
    assert(result.has_value());

    const TPacketCGMove* packet = reinterpret_cast<const TPacketCGMove*>(result.value().data());
    
    assert(packet->bRot == 70); // 350.0f / 5.0f
}

int main() {
    TestEncodeBasic();
    TestEncodeNegativeRotation();
    TestEncodeLargeRotation();
    TestEncodeLargeNegativeRotation();
    std::cout << "All tests passed for MovementCommandEncoder." << std::endl;
    return 0;
}
