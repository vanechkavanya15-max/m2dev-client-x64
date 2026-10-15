#include <gtest/gtest.h>
#include "../src/Client/Network/Handlers/GuildHandler.h"
#include "../src/Client/Gameplay/SocialDomain.h"
#include "../src/UserInterface/Packet.h"
#include "../src/UserInterface/Core/EventBus.h"
#include <vector>

using namespace Client::Network::Handlers;
using namespace Client::Gameplay;
using namespace EterBase;

class GuildHandlerTest : public ::testing::Test {
protected:
    SocialManager socialManager;
    GuildHandler handler{socialManager};
    
    void SetUp() override {
        socialManager.SetGuild(std::make_shared<Guild>(GuildId{1}, "TestGuild", 1));
    }
};

TEST_F(GuildHandlerTest, HandleMarkUpdatePacket_Success) {
    TPacketGCMarkUpdate packet{};
    packet.header = 0;
    packet.length = sizeof(TPacketGCMarkUpdate);
    packet.guildID = 1;
    packet.imgIdx = 42;
    
    std::vector<uint8_t> payload(reinterpret_cast<uint8_t*>(&packet), reinterpret_cast<uint8_t*>(&packet) + sizeof(packet));
    
    bool eventFired = false;
    auto subId = UserInterface::Core::EventBus::GetInstance().Subscribe<GuildMarkUpdatedEvent>(
        [&](const GuildMarkUpdatedEvent& ev) {
            EXPECT_EQ(ev.guildId, 1);
            EXPECT_EQ(ev.markIndex, 42);
            eventFired = true;
        }
    );
    
    auto result = handler.HandleMarkUpdatePacket(payload);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(socialManager.GetGuild()->GetMark(), "42");
    EXPECT_TRUE(eventFired);
    
    UserInterface::Core::EventBus::GetInstance().Unsubscribe<GuildMarkUpdatedEvent>(subId);
}

TEST_F(GuildHandlerTest, HandleGuildPacket_MoneyChange_Success) {
    TPacketGCGuild packet{};
    packet.header = 0;
    packet.subheader = GuildSub::GC::MONEY_CHANGE;
    
    uint32_t money = 5000;
    packet.length = sizeof(TPacketGCGuild) + sizeof(money);
    
    std::vector<uint8_t> payload;
    payload.resize(packet.length);
    std::memcpy(payload.data(), &packet, sizeof(packet));
    std::memcpy(payload.data() + sizeof(packet), &money, sizeof(money));
    
    bool eventFired = false;
    auto subId = UserInterface::Core::EventBus::GetInstance().Subscribe<GuildBankUpdatedEvent>(
        [&](const GuildBankUpdatedEvent& ev) {
            EXPECT_EQ(ev.gold, 5000);
            eventFired = true;
        }
    );
    
    auto result = handler.HandleGuildPacket(payload);
    EXPECT_TRUE(result.has_value());
    
    auto guild = socialManager.GetGuild();
    ASSERT_TRUE(guild);
    EXPECT_EQ(guild->GetBank(), 5000);
    EXPECT_TRUE(eventFired);
    
    UserInterface::Core::EventBus::GetInstance().Unsubscribe<GuildBankUpdatedEvent>(subId);
}

TEST_F(GuildHandlerTest, HandleGuildPacket_ChangeExp_Success) {
    TPacketGCGuild packet{};
    packet.header = 0;
    packet.subheader = GuildSub::GC::CHANGE_EXP;
    
    uint8_t level = 10;
    uint32_t exp = 10000;
    packet.length = sizeof(TPacketGCGuild) + sizeof(level) + sizeof(exp);
    
    std::vector<uint8_t> payload;
    payload.resize(packet.length);
    std::memcpy(payload.data(), &packet, sizeof(packet));
    std::memcpy(payload.data() + sizeof(packet), &level, sizeof(level));
    std::memcpy(payload.data() + sizeof(packet) + sizeof(level), &exp, sizeof(exp));
    
    bool eventFired = false;
    auto subId = UserInterface::Core::EventBus::GetInstance().Subscribe<GuildExpUpdatedEvent>(
        [&](const GuildExpUpdatedEvent& ev) {
            EXPECT_EQ(ev.level, 10);
            EXPECT_EQ(ev.exp, 10000);
            eventFired = true;
        }
    );
    
    auto result = handler.HandleGuildPacket(payload);
    EXPECT_TRUE(result.has_value());
    
    auto guild = socialManager.GetGuild();
    ASSERT_TRUE(guild);
    auto expPair = guild->GetExp();
    EXPECT_EQ(expPair.first, 10);
    EXPECT_EQ(expPair.second, 10000);
    EXPECT_TRUE(eventFired);
    
    UserInterface::Core::EventBus::GetInstance().Unsubscribe<GuildExpUpdatedEvent>(subId);
}

TEST_F(GuildHandlerTest, HandleGuildPacket_List_Success) {
    TPacketGCGuild packet{};
    packet.header = 0;
    packet.subheader = GuildSub::GC::LIST;
    
    TPacketGCGuildSubMember member{};
    member.pid = 123;
    member.byGrade = 1;
    member.byNameFlag = 1;
    
    char name[CHARACTER_NAME_MAX_LEN + 1] = "PlayerName";
    
    packet.length = sizeof(TPacketGCGuild) + sizeof(TPacketGCGuildSubMember) + sizeof(name);
    
    std::vector<uint8_t> payload;
    payload.resize(packet.length);
    std::memcpy(payload.data(), &packet, sizeof(packet));
    std::memcpy(payload.data() + sizeof(packet), &member, sizeof(member));
    std::memcpy(payload.data() + sizeof(packet) + sizeof(member), name, sizeof(name));
    
    auto result = handler.HandleGuildPacket(payload);
    EXPECT_TRUE(result.has_value());
    
    auto guild = socialManager.GetGuild();
    ASSERT_TRUE(guild);
    
    auto addedMember = guild->GetMember(EntityId{123});
    ASSERT_TRUE(addedMember.has_value());
    EXPECT_EQ(addedMember->name, "PlayerName");
}

TEST_F(GuildHandlerTest, HandleMarkUpdatePacket_BufferUnderflow) {
    std::vector<uint8_t> payload(sizeof(TPacketGCMarkUpdate) - 1);
    auto result = handler.HandleMarkUpdatePacket(payload);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), EterBase::PacketError::BufferUnderflow);
}
