#define CATCH_CONFIG_MAIN

#include "src/Client/Network/Handlers/LoadingPacketHandler.h"
#include "src/UserInterface/Packet.h"
#include <cassert>
#include <vector>
#include <cstring>
#include <iostream>

void Test_BufferUnderflow() {
    Client::Network::Handlers::LoadingPacketHandler handler;
    Client::Core::WorldContext context;

    std::vector<uint8_t> payload(sizeof(TPacketGCMainCharacter) - 1, 0);

    auto result = handler.HandleMainCharacter(payload, context);

    assert(!result.has_value());
    assert(result.error() == Client::Core::PacketError::BufferUnderflow);
    std::cout << "Test_BufferUnderflow passed.\n";
}

void Test_HandleMainCharacter_Success() {
    Client::Network::Handlers::LoadingPacketHandler handler;
    Client::Core::WorldContext context;

    TPacketGCMainCharacter packet{};
    packet.header = GC::MAIN_CHARACTER;
    packet.length = sizeof(TPacketGCMainCharacter);
    packet.dwVID = 12345;
    packet.lX = 1000;
    packet.lY = 2000;
    packet.lZ = 3000;
    packet.wRaceNum = 1;
    packet.fBGMVol = 1.0f;
    packet.byEmpire = 2;
    packet.bySkillGroup = 3;
    std::strncpy(packet.szName, "TestPlayer", sizeof(packet.szName) - 1);
    std::strncpy(packet.szBGMName, "test.mp3", sizeof(packet.szBGMName) - 1);

    std::vector<uint8_t> payload(sizeof(packet));
    std::memcpy(payload.data(), &packet, sizeof(packet));

    auto result = handler.HandleMainCharacter(payload, context);

    assert(result.has_value());
    assert(context.localPlayerVid.get() == 12345);
    assert(context.localPlayerCoords.x == 1000.0f);
    assert(context.localPlayerCoords.y == 2000.0f);
    assert(context.localPlayerCoords.z == 3000.0f);
    std::cout << "Test_HandleMainCharacter_Success passed.\n";
}

int main() {
    Test_BufferUnderflow();
    Test_HandleMainCharacter_Success();
    std::cout << "All tests passed.\n";
    return 0;
}
