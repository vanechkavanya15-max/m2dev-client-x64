#include <iostream>
#include <cassert>
#include "../src/Client/Gameplay/SocialDomain.h"

using namespace Client::Gameplay;

void TestParty() {
    Party party;
    assert(party.IsEmpty());

    auto e1 = EterBase::EntityId(1);
    auto e2 = EterBase::EntityId(2);

    PartyMember m1(e1, "Player1");
    auto res1 = party.AddMember(m1);
    assert(res1.has_value());
    assert(party.GetLeader().value() == e1);
    assert(!party.IsEmpty());

    PartyMember m2(e2, "Player2");
    auto res2 = party.AddMember(m2);
    assert(res2.has_value());
    assert(party.GetMembers().size() == 2);

    auto updateHPRes = party.UpdateMemberHP(e2, 50);
    assert(updateHPRes.has_value());
    assert(party.GetMember(e2).value().hpPercentage == 50);

    auto updateDistRes = party.UpdateMemberDistance(e1, 15.5f);
    assert(updateDistRes.has_value());
    assert(party.GetMember(e1).value().distance == 15.5f);

    auto setLeaderRes = party.SetLeader(e2);
    assert(setLeaderRes.has_value());
    assert(party.GetLeader().value() == e2);

    auto removeRes = party.RemoveMember(e2);
    assert(removeRes.has_value());
    assert(party.GetMembers().size() == 1);
    assert(party.GetLeader().value() == e1);

    std::cout << "[OK] TestParty passed.\n";
}

void TestGuild() {
    auto gId = EterBase::GuildId(10);
    Guild guild(gId, "Warriors", 1);
    assert(guild.GetId() == gId);
    assert(guild.GetName() == "Warriors");
    assert(guild.GetRank() == 1);

    auto e1 = EterBase::EntityId(100);
    GuildMember gm1(e1, "GuildMaster", 0xFFFFFFFF);
    auto resAdd = guild.AddMember(gm1);
    assert(resAdd.has_value());
    assert(guild.GetMembers().size() == 1);

    auto updatePermRes = guild.UpdateMemberPermissions(e1, 0x00000001);
    assert(updatePermRes.has_value());
    assert(guild.GetMember(e1).value().permissions == 0x00000001);

    auto resRem = guild.RemoveMember(e1);
    assert(resRem.has_value());
    assert(guild.GetMembers().empty());

    guild.SetMark("mark_data_string");
    assert(guild.GetMark() == "mark_data_string");

    std::cout << "[OK] TestGuild passed.\n";
}

void TestSocialManager() {
    SocialManager sm;
    
    // Friends / Ignore
    auto resF1 = sm.AddFriend("Alice");
    assert(resF1.has_value());
    assert(sm.IsFriend("Alice"));

    auto resF2 = sm.AddFriend("Alice");
    assert(!resF2.has_value());

    auto resI1 = sm.AddIgnored("Bob");
    assert(resI1.has_value());
    assert(sm.IsIgnored("Bob"));

    auto resF3 = sm.AddFriend("Bob");
    assert(!resF3.has_value());

    auto resRmF = sm.RemoveFriend("Alice");
    assert(resRmF.has_value());
    assert(!sm.IsFriend("Alice"));

    auto resRmI = sm.RemoveIgnored("Bob");
    assert(resRmI.has_value());
    assert(!sm.IsIgnored("Bob"));

    // Party Management
    sm.CreateParty();
    assert(sm.GetParty() != nullptr);
    sm.LeaveParty();
    assert(sm.GetParty() == nullptr);

    // Guild Management
    auto guild = std::make_shared<Guild>(EterBase::GuildId(5), "Knights", 2);
    sm.SetGuild(guild);
    assert(sm.GetGuild() != nullptr);
    sm.LeaveGuild();
    assert(sm.GetGuild() == nullptr);

    std::cout << "[OK] TestSocialManager passed.\n";
}

int main() {
    std::cout << "Running SocialDomain Tests...\n";
    TestParty();
    TestGuild();
    TestSocialManager();
    std::cout << "All tests passed successfully.\n";
    return 0;
}
