#include <gtest/gtest.h>
#include "../src/Client/Network/Handlers/ActorMoveHandler.h"
#include "../src/Client/World/SpatialHashGrid.h"
#include "../src/UserInterface/InstanceBase.h"

using namespace Client::Network;
using namespace Client::World;
using namespace EterBase;

class ActorMoveHandlerTest : public ::testing::Test {
protected:
    SpatialHashGrid grid{100.0f};

    void SetUp() override {
        // Init state if necessary
    }
};

TEST_F(ActorMoveHandlerTest, Handle_NullPacket_ReturnsError) {
    auto result = ActorMoveHandler::Handle(nullptr, grid);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), PacketError::MalformedPayload);
}

TEST_F(ActorMoveHandlerTest, Handle_InvalidFunc_ReturnsError) {
    TPacketGCMove packet{};
    packet.bFunc = 99; // Invalid

    auto result = ActorMoveHandler::Handle(&packet, grid);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), PacketError::MalformedPayload);
}

TEST_F(ActorMoveHandlerTest, Handle_ValidWalk_UpdatesGrid) {
    TPacketGCMove packet{};
    packet.dwVID = 12345;
    packet.lX = 500;
    packet.lY = 600;
    packet.bFunc = CInstanceBase::FUNC_MOVE; // Valid

    auto result = ActorMoveHandler::Handle(&packet, grid);
    EXPECT_TRUE(result.has_value());

    auto queried = grid.QueryRadius(500.0f, 600.0f, 10.0f);
    EXPECT_EQ(queried.size(), 1);
    EXPECT_EQ(queried[0].value(), 12345);
}

TEST_F(ActorMoveHandlerTest, Handle_ValidWait_UpdatesGrid) {
    TPacketGCMove packet{};
    packet.dwVID = 67890;
    packet.lX = -200;
    packet.lY = 300;
    packet.bFunc = CInstanceBase::FUNC_WAIT; // Valid

    auto result = ActorMoveHandler::Handle(&packet, grid);
    EXPECT_TRUE(result.has_value());

    auto queried = grid.QueryRadius(-200.0f, 300.0f, 10.0f);
    EXPECT_EQ(queried.size(), 1);
    EXPECT_EQ(queried[0].value(), 67890);
}
