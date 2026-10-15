#include <gtest/gtest.h>
#include <vector>
#include <span>

#include "../src/Client/Network/Handlers/DeadHandler.h"
#include "../src/Client/Network/CombatPacketCodec.h"
#include "../src/UserInterface/Packet.h"

using namespace Client::Network::Handlers;
using namespace Client::World;

class DeadHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        capturedVid = EntityVid{0};
        capturedState = World::MotionState::Count;
        callbackCalled = false;
    }

    EntityVid capturedVid{0};
    World::MotionState capturedState{World::MotionState::Count};
    bool callbackCalled = false;
};

TEST_F(DeadHandlerTest, ProcessDeadPacket_SuccessfulDecoding_CallsCallbackWithCorrectData) {
    DeadHandler handler([this](EntityVid vid, World::MotionState state) {
        capturedVid = vid;
        capturedState = state;
        callbackCalled = true;
    });

    TPacketGCDead packet;
    packet.header = 1; // dummy header
    packet.length = sizeof(TPacketGCDead);
    packet.vid = 12345;

    std::vector<uint8_t> buffer(sizeof(TPacketGCDead));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketGCDead));

    auto result = handler.ProcessDeadPacket(buffer);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(capturedVid.value(), 12345);
    EXPECT_EQ(capturedState, World::MotionState::Dead);
}

TEST_F(DeadHandlerTest, ProcessDeadPacket_BufferUnderflow_ReturnsError) {
    DeadHandler handler([this](EntityVid vid, World::MotionState state) {
        callbackCalled = true;
    });

    std::vector<uint8_t> buffer(sizeof(TPacketGCDead) - 1); // Too small

    auto result = handler.ProcessDeadPacket(buffer);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), EterBase::PacketError::BufferUnderflow);
    EXPECT_FALSE(callbackCalled);
}

TEST_F(DeadHandlerTest, ProcessDeadPacket_NoCallback_SucceedsWithoutCrashing) {
    DeadHandler handler(nullptr);

    TPacketGCDead packet;
    packet.header = 1;
    packet.length = sizeof(TPacketGCDead);
    packet.vid = 54321;

    std::vector<uint8_t> buffer(sizeof(TPacketGCDead));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketGCDead));

    auto result = handler.ProcessDeadPacket(buffer);

    ASSERT_TRUE(result.has_value());
}
