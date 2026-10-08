#include <gtest/gtest.h>
#include "../src/Client/Simulation/VirtualPacketGenerator.h"

#include <cstring>
#include <map>

using namespace Client::Simulation;

TEST(VirtualPacketGeneratorTest, CreateActorSpawnPacket_Layout)
{
    auto packetData = VirtualPacketGenerator::CreateActorSpawnPacket(1001, 101, 5000, 6000, 100, 100, 200);

    // Verify size
    EXPECT_EQ(packetData.size(), 38);

    // Verify header (bytes 0-1)
    uint16_t header = 0;
    std::memcpy(&header, packetData.data(), sizeof(header));
    EXPECT_EQ(header, 1); // HEADER_GC_CHARACTER_ADD

    // Verify length (bytes 2-3)
    uint16_t length = 0;
    std::memcpy(&length, packetData.data() + 2, sizeof(length));
    EXPECT_EQ(length, 38);

    // Verify dwVID (bytes 4-7)
    uint32_t vid = 0;
    std::memcpy(&vid, packetData.data() + 4, sizeof(vid));
    EXPECT_EQ(vid, 1001);

    // Verify vnum / wRaceNum (bytes 21-22) 
    // Layout:
    // header(2) + length(2) + vid(4) + angle(4) + x(4) + y(4) + z(4) + bType(1) + wRaceNum(2)
    // 2+2+4+4+4+4+4+1 = 25 -> offset 25
    uint16_t raceNum = 0;
    std::memcpy(&raceNum, packetData.data() + 25, sizeof(raceNum));
    EXPECT_EQ(raceNum, 101);
}

TEST(VirtualPacketGeneratorTest, CreateActorMovePacket_Layout)
{
    auto packetData = VirtualPacketGenerator::CreateActorMovePacket(2002, 100, 200, 500, 600, 3); // 3 = FUNC_MOVE

    EXPECT_EQ(packetData.size(), 27);

    uint16_t header = 0;
    std::memcpy(&header, packetData.data(), sizeof(header));
    EXPECT_EQ(header, 3); // HEADER_GC_CHARACTER_MOVE

    uint16_t length = 0;
    std::memcpy(&length, packetData.data() + 2, sizeof(length));
    EXPECT_EQ(length, 27);

    // Layout: header(2) + length(2) + bFunc(1) + bArg(1) + bRot(1) + dwVID(4) + lX(4) + lY(4) + dwTime(4) + dwDuration(4)
    uint8_t func = 0;
    std::memcpy(&func, packetData.data() + 4, sizeof(func));
    EXPECT_EQ(func, 3);

    uint32_t vid = 0;
    std::memcpy(&vid, packetData.data() + 7, sizeof(vid));
    EXPECT_EQ(vid, 2002);
}

TEST(VirtualPacketGeneratorTest, CreateActorDeletePacket_Layout)
{
    auto packetData = VirtualPacketGenerator::CreateActorDeletePacket(3003);

    EXPECT_EQ(packetData.size(), 8);

    uint16_t header = 0;
    std::memcpy(&header, packetData.data(), sizeof(header));
    EXPECT_EQ(header, 2); // HEADER_GC_CHARACTER_DEL

    uint16_t length = 0;
    std::memcpy(&length, packetData.data() + 2, sizeof(length));
    EXPECT_EQ(length, 8);

    uint32_t vid = 0;
    std::memcpy(&vid, packetData.data() + 4, sizeof(vid));
    EXPECT_EQ(vid, 3003);
}

TEST(VirtualPacketGeneratorTest, CreateDamageInfoPacket_Layout)
{
    auto packetData = VirtualPacketGenerator::CreateDamageInfoPacket(1001, 2002, 550, 1);

    EXPECT_EQ(packetData.size(), 13);

    uint16_t header = 0;
    std::memcpy(&header, packetData.data(), sizeof(header));
    EXPECT_EQ(header, 135); // HEADER_GC_DAMAGE_INFO

    uint16_t length = 0;
    std::memcpy(&length, packetData.data() + 2, sizeof(length));
    EXPECT_EQ(length, 13);

    uint32_t vid = 0;
    std::memcpy(&vid, packetData.data() + 4, sizeof(vid));
    EXPECT_EQ(vid, 2002);

    int32_t damage = 0;
    std::memcpy(&damage, packetData.data() + 9, sizeof(damage));
    EXPECT_EQ(damage, 550);
}

TEST(VirtualPacketGeneratorTest, CreateItemGroundAddPacket_Layout)
{
    auto packetData = VirtualPacketGenerator::CreateItemGroundAddPacket(5005, 2799, 1000, 2000, 1001);

    EXPECT_EQ(packetData.size(), 24);

    uint16_t header = 0;
    std::memcpy(&header, packetData.data(), sizeof(header));
    EXPECT_EQ(header, 26); // HEADER_GC_ITEM_GROUND_ADD

    uint16_t length = 0;
    std::memcpy(&length, packetData.data() + 2, sizeof(length));
    EXPECT_EQ(length, 24);
}

TEST(VirtualPacketGeneratorTest, CreateItemGroundDelPacket_Layout)
{
    auto packetData = VirtualPacketGenerator::CreateItemGroundDelPacket(5005);

    EXPECT_EQ(packetData.size(), 8);

    uint16_t header = 0;
    std::memcpy(&header, packetData.data(), sizeof(header));
    EXPECT_EQ(header, 27); // HEADER_GC_ITEM_GROUND_DEL

    uint16_t length = 0;
    std::memcpy(&length, packetData.data() + 2, sizeof(length));
    EXPECT_EQ(length, 8);
}

TEST(VirtualPacketGeneratorTest, CreatePlayerPointsPacket_Layout)
{
    std::map<uint8_t, int32_t> points = {{1, 100}, {5, 500}, {254, 999}};
    auto packetData = VirtualPacketGenerator::CreatePlayerPointsPacket(points);

    EXPECT_EQ(packetData.size(), 1024);

    uint16_t header = 0;
    std::memcpy(&header, packetData.data(), sizeof(header));
    EXPECT_EQ(header, 16); // HEADER_GC_PLAYER_POINTS

    uint16_t length = 0;
    std::memcpy(&length, packetData.data() + 2, sizeof(length));
    EXPECT_EQ(length, 1024);

    // Verify points
    // header(2) + length(2) + points[255 * 4](1020)
    int32_t point1 = 0;
    std::memcpy(&point1, packetData.data() + 4 + (1 * sizeof(int32_t)), sizeof(point1));
    EXPECT_EQ(point1, 100);

    int32_t point5 = 0;
    std::memcpy(&point5, packetData.data() + 4 + (5 * sizeof(int32_t)), sizeof(point5));
    EXPECT_EQ(point5, 500);

    int32_t point254 = 0;
    std::memcpy(&point254, packetData.data() + 4 + (254 * sizeof(int32_t)), sizeof(point254));
    EXPECT_EQ(point254, 999);
}
