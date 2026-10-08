#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/Client/Network/DynamicPacketFramerV2.h"

using namespace Client::Network;

TEST_CASE("DynamicPacketFramerV2 - Fixed Size") {
    DynamicPacketFramerV2 framer;
    framer.RegisterFixedSize(0x01, 5);

    SUBCASE("Complete Packet") {
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
        framer.AppendData(data);

        auto result = framer.FrameNextPacket();
        REQUIRE(result.has_value());
        REQUIRE(result.value().size() == 5);
        REQUIRE(result.value()[0] == 0x01);
    }

    SUBCASE("Fragmented Packet 1 Byte At A Time") {
        std::vector<uint8_t> full_data = {0x01, 0x02, 0x03, 0x04, 0x05};
        
        for (size_t i = 0; i < full_data.size(); ++i) {
            std::vector<uint8_t> chunk = { full_data[i] };
            framer.AppendData(chunk);
            
            auto result = framer.FrameNextPacket();
            if (i < full_data.size() - 1) {
                REQUIRE(!result.has_value());
                REQUIRE(result.error() == EterBase::PacketError::BufferUnderflow);
            } else {
                REQUIRE(result.has_value());
                REQUIRE(result.value().size() == 5);
                REQUIRE(result.value()[0] == 0x01);
                REQUIRE(result.value()[4] == 0x05);
            }
        }
    }
}

TEST_CASE("DynamicPacketFramerV2 - Dynamic Size") {
    DynamicPacketFramerV2 framer;
    // Opcode 0x02, length offset = 1, length size = 2, max size = 100
    framer.RegisterDynamicSize(0x02, 1, 2, 100);

    SUBCASE("Complete Dynamic Packet") {
        std::vector<uint8_t> data = {0x02, 0x06, 0x00, 0xAA, 0xBB, 0xCC}; // Length = 6
        framer.AppendData(data);

        auto result = framer.FrameNextPacket();
        REQUIRE(result.has_value());
        REQUIRE(result.value().size() == 6);
        REQUIRE(result.value()[3] == 0xAA);
    }

    SUBCASE("Fragmented Length Field") {
        std::vector<uint8_t> data1 = {0x02, 0x06}; // Missing the second byte of length
        framer.AppendData(data1);

        auto result1 = framer.FrameNextPacket();
        REQUIRE(!result1.has_value());
        REQUIRE(result1.error() == EterBase::PacketError::BufferUnderflow);

        std::vector<uint8_t> data2 = {0x00, 0xAA, 0xBB, 0xCC};
        framer.AppendData(data2);

        auto result2 = framer.FrameNextPacket();
        REQUIRE(result2.has_value());
        REQUIRE(result2.value().size() == 6);
    }

    SUBCASE("Fragmented Dynamic Packet 1 Byte At A Time") {
        std::vector<uint8_t> full_data = {0x02, 0x06, 0x00, 0xAA, 0xBB, 0xCC};
        
        for (size_t i = 0; i < full_data.size(); ++i) {
            std::vector<uint8_t> chunk = { full_data[i] };
            framer.AppendData(chunk);
            
            auto result = framer.FrameNextPacket();
            if (i < full_data.size() - 1) {
                REQUIRE(!result.has_value());
                REQUIRE(result.error() == EterBase::PacketError::BufferUnderflow);
            } else {
                REQUIRE(result.has_value());
                REQUIRE(result.value().size() == 6);
                REQUIRE(result.value()[0] == 0x02);
                REQUIRE(result.value()[5] == 0xCC);
            }
        }
    }

    SUBCASE("Malformed Payload - Size Exceeds Max") {
        std::vector<uint8_t> data = {0x02, 0x65, 0x00}; // Length = 101, max = 100
        framer.AppendData(data);

        auto result = framer.FrameNextPacket();
        REQUIRE(!result.has_value());
        REQUIRE(result.error() == EterBase::PacketError::MalformedPayload);
    }
}

TEST_CASE("DynamicPacketFramerV2 - Continuous Stream Multiple Packets") {
    DynamicPacketFramerV2 framer;
    framer.RegisterFixedSize(0x01, 3);
    framer.RegisterDynamicSize(0x02, 1, 1, 50);

    std::vector<uint8_t> data = {
        0x01, 0xAA, 0xBB,             // Packet 1: Fixed, length 3
        0x02, 0x04, 0xCC, 0xDD,       // Packet 2: Dynamic, length 4 (0x02, 0x04, 0xCC, 0xDD)
        0x01, 0xEE, 0xFF              // Packet 3: Fixed, length 3
    };

    framer.AppendData(data);

    auto res1 = framer.FrameNextPacket();
    REQUIRE(res1.has_value());
    REQUIRE(res1.value().size() == 3);
    REQUIRE(res1.value()[0] == 0x01);

    auto res2 = framer.FrameNextPacket();
    REQUIRE(res2.has_value());
    REQUIRE(res2.value().size() == 4);
    REQUIRE(res2.value()[0] == 0x02);
    REQUIRE(res2.value()[2] == 0xCC);

    auto res3 = framer.FrameNextPacket();
    REQUIRE(res3.has_value());
    REQUIRE(res3.value().size() == 3);
    REQUIRE(res3.value()[0] == 0x01);

    auto res4 = framer.FrameNextPacket();
    REQUIRE(!res4.has_value());
    REQUIRE(res4.error() == EterBase::PacketError::BufferUnderflow);
}

TEST_CASE("DynamicPacketFramerV2 - Unknown Opcode") {
    DynamicPacketFramerV2 framer;
    framer.RegisterFixedSize(0x01, 5);

    std::vector<uint8_t> data = {0xFF, 0x00, 0x00};
    framer.AppendData(data);

    auto result = framer.FrameNextPacket();
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == EterBase::PacketError::UnknownOpcode);
}
