#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <Client/Gameplay/QuestCommandHandler.h>
#include <Client/Core/DomainCommands.h>
#include <UserInterface/Packet.h>

class MockNetworkPort : public Client::Core::INetworkPort {
public:
    MOCK_METHOD(Client::Core::Result<void, EterBase::PacketError>, SendRaw, (uint8_t opcode, std::span<const uint8_t> payload), (override));
    MOCK_METHOD(bool, IsConnected, (), (const, noexcept, override));
};

class QuestCommandHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockPort = std::make_shared<testing::NiceMock<MockNetworkPort>>();
        ON_CALL(*mockPort, IsConnected()).WillByDefault(testing::Return(true));
        handler = std::make_unique<Client::Gameplay::QuestCommandHandler>(mockPort);
    }

    std::shared_ptr<MockNetworkPort> mockPort;
    std::unique_ptr<Client::Gameplay::QuestCommandHandler> handler;
};

TEST_F(QuestCommandHandlerTest, AnswerWithoutDialogReturnsError) {
    Client::Core::QuestAnswerCommand cmd{0};
    auto result = handler->Execute(cmd);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Client::Core::CommandError::InvalidParameter);
}

TEST_F(QuestCommandHandlerTest, AnswerWithInvalidIndexReturnsError) {
    handler->SetActiveScriptDialog(2); // Answers: 0, 1
    Client::Core::QuestAnswerCommand cmd{2};
    auto result = handler->Execute(cmd);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Client::Core::CommandError::InvalidParameter);
}

TEST_F(QuestCommandHandlerTest, AnswerDisconnectedReturnsError) {
    handler->SetActiveScriptDialog(2);
    EXPECT_CALL(*mockPort, IsConnected()).WillOnce(testing::Return(false));
    Client::Core::QuestAnswerCommand cmd{1};
    auto result = handler->Execute(cmd);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Client::Core::CommandError::Disconnected);
}

TEST_F(QuestCommandHandlerTest, AnswerValidSendsPacketAndClearsState) {
    handler->SetActiveScriptDialog(2);
    Client::Core::QuestAnswerCommand cmd{1};

    EXPECT_CALL(*mockPort, SendRaw(CG::SCRIPT_ANSWER, testing::_))
        .WillOnce([](uint8_t opcode, std::span<const uint8_t> payload) {
            EXPECT_EQ(payload.size(), sizeof(TPacketCGScriptAnswer));
            auto* packet = reinterpret_cast<const TPacketCGScriptAnswer*>(payload.data());
            EXPECT_EQ(packet->header, CG::SCRIPT_ANSWER);
            EXPECT_EQ(packet->answer, 1);
            return Client::Core::Result<void, EterBase::PacketError>{};
        });

    auto result = handler->Execute(cmd);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(handler->HasActiveScriptDialog());
}

TEST_F(QuestCommandHandlerTest, ConfirmWithoutDialogReturnsError) {
    Client::Core::QuestConfirmCommand cmd{1, 1234};
    auto result = handler->Execute(cmd);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Client::Core::CommandError::InvalidParameter);
}

TEST_F(QuestCommandHandlerTest, ConfirmWithMismatchedPIDReturnsError) {
    handler->SetActiveConfirmDialog(1234);
    Client::Core::QuestConfirmCommand cmd{1, 9999};
    auto result = handler->Execute(cmd);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Client::Core::CommandError::InvalidParameter);
}

TEST_F(QuestCommandHandlerTest, ConfirmDisconnectedReturnsError) {
    handler->SetActiveConfirmDialog(1234);
    EXPECT_CALL(*mockPort, IsConnected()).WillOnce(testing::Return(false));
    Client::Core::QuestConfirmCommand cmd{1, 1234};
    auto result = handler->Execute(cmd);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Client::Core::CommandError::Disconnected);
}

TEST_F(QuestCommandHandlerTest, ConfirmValidSendsPacketAndClearsState) {
    handler->SetActiveConfirmDialog(1234);
    Client::Core::QuestConfirmCommand cmd{1, 1234};

    EXPECT_CALL(*mockPort, SendRaw(CG::QUEST_CONFIRM, testing::_))
        .WillOnce([](uint8_t opcode, std::span<const uint8_t> payload) {
            EXPECT_EQ(payload.size(), sizeof(TPacketCGQuestConfirm));
            auto* packet = reinterpret_cast<const TPacketCGQuestConfirm*>(payload.data());
            EXPECT_EQ(packet->header, CG::QUEST_CONFIRM);
            EXPECT_EQ(packet->answer, 1);
            EXPECT_EQ(packet->requestPID, 1234);
            return Client::Core::Result<void, EterBase::PacketError>{};
        });

    auto result = handler->Execute(cmd);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(handler->HasActiveConfirmDialog());
}
