#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/Client/Simulation/MockNetworkPortAdvanced.h"
#include <vector>

using namespace Client::Simulation;
using namespace Client::Core;

TEST_SUITE("MockNetworkPortAdvanced") {
    TEST_CASE("SendRaw and GetSentPackets") {
        MockNetworkPortAdvanced port;
        std::vector<uint8_t> payload = {1, 2, 3};
        auto result = port.SendRaw(10, payload);
        
        CHECK(result.has_value());
        REQUIRE(port.GetSentPackets().size() == 1);
        CHECK(port.GetSentPackets()[0].opcode == 10);
        CHECK(port.GetSentPackets()[0].payload == payload);
        CHECK(port.CountSentPackets(10) == 1);
        CHECK(port.GetLastOpcode() == 10);
    }

    TEST_CASE("Disconnected state") {
        MockNetworkPortAdvanced port;
        port.SetConnected(false);
        std::vector<uint8_t> payload = {1, 2, 3};
        auto result = port.SendRaw(10, payload);
        
        CHECK_FALSE(result.has_value());
        CHECK(result.error() == PacketError::Timeout);
        CHECK(port.GetSentPackets().empty());
    }

    TEST_CASE("InjectPacket and ReceiveCallback") {
        MockNetworkPortAdvanced port;
        bool callbackCalled = false;
        
        port.SetReceiveCallback([&](uint8_t opcode, std::span<const uint8_t> payload) {
            callbackCalled = true;
            CHECK(opcode == 20);
            CHECK(payload.size() == 2);
            CHECK(payload[0] == 4);
            CHECK(payload[1] == 5);
        });

        std::vector<uint8_t> payload = {4, 5};
        port.InjectPacket(20, payload);
        
        CHECK(callbackCalled);
        REQUIRE(port.GetInjectedPackets().size() == 1);
        CHECK(port.GetInjectedPackets()[0].opcode == 20);
        CHECK(port.GetInjectedPackets()[0].payload == payload);
    }

    TEST_CASE("AutoResponder") {
        MockNetworkPortAdvanced port;
        bool responderCalled = false;
        
        port.RegisterAutoResponder(30, [&](std::span<const uint8_t> payload) {
            responderCalled = true;
            std::vector<uint8_t> responsePayload = {9, 9};
            port.InjectPacket(40, responsePayload);
        });

        std::vector<uint8_t> payload = {1};
        auto result = port.SendRaw(30, payload);
        
        CHECK(result.has_value());
        CHECK(responderCalled);
        REQUIRE(port.GetInjectedPackets().size() == 1);
        CHECK(port.GetInjectedPackets()[0].opcode == 40);
        CHECK(port.GetInjectedPackets()[0].payload == std::vector<uint8_t>{9, 9});
    }
}
