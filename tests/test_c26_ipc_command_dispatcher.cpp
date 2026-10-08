#include <gtest/gtest.h>
#include "Client/IPC/IPCCommandDispatcher.h"
#include "Client/Core/GameSession.h"
#include "UserInterface/Core/EventBus.h"
#include <cmath>

using namespace Client::IPC;
using namespace Client::Core;
using namespace UserInterface::Core;

class IPCCommandDispatcherTest : public ::testing::Test {
protected:
    EventBus& eventBus = EventBus::Instance();
    GameSession gameSession;
    IPCCommandDispatcher dispatcher{eventBus, &gameSession};

    void SetUp() override {
        // Clear subscriptions before each test by resetting the eventBus state if possible,
        // but EventBus is a singleton in this context. We will just subscribe locally.
    }
    
    void TearDown() override {
        // Clean up
    }
};

TEST_F(IPCCommandDispatcherTest, HandleGotoValid) {
    DecodedCommand cmd;
    cmd.opcode = IpcOpcode::Goto;
    cmd.payload = IpcGotoPayload{MapCoords{100.0f, 200.0f, 0.0f}};

    auto result = dispatcher.Dispatch(cmd);
    EXPECT_TRUE(result.has_value());
    // In a real mock we could verify gameSession.Execute was called, 
    // but here we just ensure it returns success without error.
}

TEST_F(IPCCommandDispatcherTest, HandleGotoInvalidCoordinates) {
    DecodedCommand cmd;
    cmd.opcode = IpcOpcode::Goto;
    cmd.payload = IpcGotoPayload{MapCoords{std::nanf(""), 200.0f, 0.0f}};

    auto result = dispatcher.Dispatch(cmd);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), IpcDispatchError::InvalidCoordinates);
}

TEST_F(IPCCommandDispatcherTest, HandleAttackTargetValid) {
    DecodedCommand cmd;
    cmd.opcode = IpcOpcode::AttackTarget;
    cmd.payload = IpcAttackPayload{EntityVid{1234}};

    auto result = dispatcher.Dispatch(cmd);
    EXPECT_TRUE(result.has_value());
}

TEST_F(IPCCommandDispatcherTest, HandleAttackTargetZeroVid) {
    DecodedCommand cmd;
    cmd.opcode = IpcOpcode::AttackTarget;
    cmd.payload = IpcAttackPayload{EntityVid{0}};

    auto result = dispatcher.Dispatch(cmd);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), IpcDispatchError::ZeroVid);
}

TEST_F(IPCCommandDispatcherTest, HandlePickupLootValid) {
    DecodedCommand cmd;
    cmd.opcode = IpcOpcode::PickupLoot;
    cmd.payload = IpcPickupPayload{EntityVid{5678}};

    auto result = dispatcher.Dispatch(cmd);
    EXPECT_TRUE(result.has_value());
}

TEST_F(IPCCommandDispatcherTest, HandlePickupLootZeroVid) {
    DecodedCommand cmd;
    cmd.opcode = IpcOpcode::PickupLoot;
    cmd.payload = IpcPickupPayload{EntityVid{0}};

    auto result = dispatcher.Dispatch(cmd);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), IpcDispatchError::ZeroVid);
}

TEST_F(IPCCommandDispatcherTest, HandleUseSkillValid) {
    DecodedCommand cmd;
    cmd.opcode = IpcOpcode::UseSkill;
    cmd.payload = IpcUseSkillPayload{SkillId{1}, EntityVid{123}};

    auto result = dispatcher.Dispatch(cmd);
    EXPECT_TRUE(result.has_value());
}

TEST_F(IPCCommandDispatcherTest, HandleUseItemValid) {
    DecodedCommand cmd;
    cmd.opcode = IpcOpcode::UseItem;
    cmd.payload = IpcUseItemPayload{ItemSlot{10}};

    auto result = dispatcher.Dispatch(cmd);
    EXPECT_TRUE(result.has_value());
}

TEST_F(IPCCommandDispatcherTest, HandleUseItemInvalidSlot) {
    DecodedCommand cmd;
    cmd.opcode = IpcOpcode::UseItem;
    cmd.payload = IpcUseItemPayload{ItemSlot{0xFFFF}};

    auto result = dispatcher.Dispatch(cmd);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), IpcDispatchError::InvalidSlot);
}

TEST_F(IPCCommandDispatcherTest, HandleSelectTargetValidEventBus) {
    DecodedCommand cmd;
    cmd.opcode = IpcOpcode::SelectTarget;
    cmd.payload = IpcSelectTargetPayload{EntityVid{999}};

    bool eventReceived = false;
    uint32_t receivedTargetId = 0;
    
    uint32_t subId = eventBus.Subscribe<TargetBoardRefreshEvent>([&](const TargetBoardRefreshEvent& e) {
        eventReceived = true;
        receivedTargetId = e.targetId;
    });

    auto result = dispatcher.Dispatch(cmd);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(eventReceived);
    EXPECT_EQ(receivedTargetId, 999u);
    
    eventBus.Unsubscribe<TargetBoardRefreshEvent>(subId);
}

TEST_F(IPCCommandDispatcherTest, HandleSelectTargetZeroVid) {
    DecodedCommand cmd;
    cmd.opcode = IpcOpcode::SelectTarget;
    cmd.payload = IpcSelectTargetPayload{EntityVid{0}};

    auto result = dispatcher.Dispatch(cmd);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), IpcDispatchError::ZeroVid);
}
