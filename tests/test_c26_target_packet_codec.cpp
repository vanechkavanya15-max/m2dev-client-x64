#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "Client/Network/TargetPacketCodec.h"
#include "EterBase/Error/PacketError.h"

// Define missing namespaces/constants for the test in case they differ.
// Note: We use GC::TARGET directly because they are defined in Packet.h which is included.

using namespace Client::Network;

TEST_CASE("TargetPacketCodec::DecodeTarget")
{
    SUBCASE("Valid packet")
    {
        TPacketGCTarget packet = {};
        packet.header = GC::TARGET;
        packet.length = sizeof(packet);
        packet.dwVID = 12345;
        packet.bHPPercent = 100;

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));

        auto result = TargetPacketCodec::DecodeTarget(buffer);
        REQUIRE(result.has_value());
        CHECK(result->dwVID == 12345);
        CHECK(result->bHPPercent == 100);
    }

    SUBCASE("Buffer underflow")
    {
        std::vector<uint8_t> buffer(sizeof(TPacketGCTarget) - 1);
        auto result = TargetPacketCodec::DecodeTarget(buffer);
        REQUIRE(!result.has_value());
        CHECK(result.error() == EterBase::PacketError::BufferUnderflow);
    }

    SUBCASE("Invalid header")
    {
        TPacketGCTarget packet = {};
        packet.header = 0xFFFF; // Invalid header
        packet.length = sizeof(packet);

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));

        auto result = TargetPacketCodec::DecodeTarget(buffer);
        REQUIRE(!result.has_value());
        CHECK(result.error() == EterBase::PacketError::InvalidHeader);
    }
}

TEST_CASE("TargetPacketCodec::DecodeTargetCreate")
{
    SUBCASE("Valid packet")
    {
        TPacketGCTargetCreate packet = {};
        packet.header = 0x0304; // Test value since exact enum not strictly checked
        packet.length = sizeof(packet);
        packet.lID = 42;
        std::strncpy(packet.szTargetName, "EnemyTarget", sizeof(packet.szTargetName) - 1);
        packet.szTargetName[sizeof(packet.szTargetName) - 1] = '\0';

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));

        auto result = TargetPacketCodec::DecodeTargetCreate(buffer);
        REQUIRE(result.has_value());
        CHECK(result->lID == 42);
        CHECK(std::string(result->szTargetName) == "EnemyTarget");
    }
}

TEST_CASE("TargetPacketCodec::DecodeTargetUpdate")
{
    SUBCASE("Valid packet")
    {
        TPacketGCTargetUpdate packet = {};
        packet.header = GC::TARGET_UPDATE;
        packet.length = sizeof(packet);
        packet.lID = 42;
        packet.lX = 100;
        packet.lY = 200;

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));

        auto result = TargetPacketCodec::DecodeTargetUpdate(buffer);
        REQUIRE(result.has_value());
        CHECK(result->lID == 42);
        CHECK(result->lX == 100);
        CHECK(result->lY == 200);
    }
}

TEST_CASE("TargetPacketCodec::DecodeTargetDelete")
{
    SUBCASE("Valid packet")
    {
        TPacketGCTargetDelete packet = {};
        packet.header = GC::TARGET_DELETE;
        packet.length = sizeof(packet);
        packet.lID = 42;

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));

        auto result = TargetPacketCodec::DecodeTargetDelete(buffer);
        REQUIRE(result.has_value());
        CHECK(result->lID == 42);
    }
}

TEST_CASE("TargetPacketCodec::DecodeCreateFly")
{
    SUBCASE("Valid packet")
    {
        TPacketGCCreateFly packet = {};
        packet.header = GC::CREATE_FLY;
        packet.length = sizeof(packet);
        packet.bType = 1;
        packet.dwStartVID = 1000;
        packet.dwEndVID = 2000;

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));

        auto result = TargetPacketCodec::DecodeCreateFly(buffer);
        REQUIRE(result.has_value());
        CHECK(result->bType == 1);
        CHECK(result->dwStartVID == 1000);
        CHECK(result->dwEndVID == 2000);
    }
}

TEST_CASE("TargetPacketCodec::EncodeFlyTargeting")
{
    SUBCASE("Valid encoding")
    {
        uint32_t targetVid = 555;
        float x = 123.4f;
        float y = 567.8f;
        float z = 0.0f; // not used by header

        auto buffer = TargetPacketCodec::EncodeFlyTargeting(targetVid, x, y, z);
        REQUIRE(buffer.size() == sizeof(TPacketCGFlyTargeting));

        TPacketCGFlyTargeting packet;
        std::memcpy(&packet, buffer.data(), sizeof(packet));

        CHECK(packet.header == CG::FLY_TARGETING);
        CHECK(packet.length == sizeof(TPacketCGFlyTargeting));
        CHECK(packet.dwTargetVID == targetVid);
        CHECK(packet.lX == static_cast<int32_t>(x));
        CHECK(packet.lY == static_cast<int32_t>(y));
    }
}
