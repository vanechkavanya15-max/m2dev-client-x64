#define D3D9_H
#define D3DX9_H

#include <iostream>
#include <cassert>
#include <memory>

// Mock for StdAfx.h requirements if compiled directly
namespace Client {
    namespace Gameplay {}
}

#include "Client/Gameplay/PartyCommandHandler.h"
#include "Client/Gameplay/SocialDomain.h"

using namespace Client::Gameplay;
using namespace EterBase;

void TestInviteSuccessNewParty() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyCommandHandler handler(socialManager);

    auto result = handler.Invite(EntityId(1), EntityId(2));
    assert(result.has_value());

    auto party = socialManager->GetParty();
    assert(party != nullptr);
    assert(party->GetMembers().size() == 2);
    
    auto leader = party->GetMember(EntityId(1));
    assert(leader->isLeader == true);
    
    auto member = party->GetMember(EntityId(2));
    assert(member->isLeader == false);
    
    std::cout << "TestInviteSuccessNewParty passed.\n";
}

void TestInviteSuccessExistingParty() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyCommandHandler handler(socialManager);

    handler.Invite(EntityId(1), EntityId(2)); // Creates party, 1 is leader
    
    auto result = handler.Invite(EntityId(1), EntityId(3)); // 1 is still leader, adds 3
    assert(result.has_value());

    auto party = socialManager->GetParty();
    assert(party->GetMembers().size() == 3);
    
    std::cout << "TestInviteSuccessExistingParty passed.\n";
}

void TestInviteFailNotLeader() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyCommandHandler handler(socialManager);

    handler.Invite(EntityId(1), EntityId(2)); // Creates party, 1 is leader
    
    // 2 is not leader, tries to invite 3
    auto result = handler.Invite(EntityId(2), EntityId(3));
    assert(!result.has_value());
    assert(result.error() == CommandError::NotLeader);
    
    std::cout << "TestInviteFailNotLeader passed.\n";
}

void TestInviteFailAlreadyInParty() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyCommandHandler handler(socialManager);

    handler.Invite(EntityId(1), EntityId(2)); 
    
    // 1 tries to invite 2 again
    auto result = handler.Invite(EntityId(1), EntityId(2));
    assert(!result.has_value());
    assert(result.error() == CommandError::AlreadyInParty);
    
    std::cout << "TestInviteFailAlreadyInParty passed.\n";
}

void TestInviteFailSelf() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyCommandHandler handler(socialManager);

    auto result = handler.Invite(EntityId(1), EntityId(1));
    assert(!result.has_value());
    assert(result.error() == CommandError::InvalidTarget);
    
    std::cout << "TestInviteFailSelf passed.\n";
}

void TestLeaveSuccess() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyCommandHandler handler(socialManager);

    handler.Invite(EntityId(1), EntityId(2));
    handler.Invite(EntityId(1), EntityId(3));
    
    auto result = handler.Leave(EntityId(2));
    assert(result.has_value());
    
    auto party = socialManager->GetParty();
    assert(party->GetMembers().size() == 2);
    assert(!party->GetMember(EntityId(2)).has_value());
    
    std::cout << "TestLeaveSuccess passed.\n";
}

void TestLeaveSuccessLeaderTransfer() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyCommandHandler handler(socialManager);

    handler.Invite(EntityId(1), EntityId(2));
    handler.Invite(EntityId(1), EntityId(3));
    
    // 1 (leader) leaves
    auto result = handler.Leave(EntityId(1));
    assert(result.has_value());
    
    auto party = socialManager->GetParty();
    assert(party->GetMembers().size() == 2);
    
    // 2 should become the new leader (it was added first after 1)
    auto newLeaderOpt = party->GetLeader();
    assert(newLeaderOpt.has_value());
    assert(newLeaderOpt.value() == EntityId(2));
    
    std::cout << "TestLeaveSuccessLeaderTransfer passed.\n";
}

void TestLeaveSuccessEmptyPartyDestroyed() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyCommandHandler handler(socialManager);

    handler.Invite(EntityId(1), EntityId(2));
    handler.Leave(EntityId(2));
    handler.Leave(EntityId(1)); // Party is now empty
    
    auto party = socialManager->GetParty();
    assert(party == nullptr);
    
    std::cout << "TestLeaveSuccessEmptyPartyDestroyed passed.\n";
}

void TestLeaveFailNotInParty() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyCommandHandler handler(socialManager);

    auto result = handler.Leave(EntityId(1)); // No party exists
    assert(!result.has_value());
    assert(result.error() == CommandError::NotInParty);
    
    std::cout << "TestLeaveFailNotInParty passed.\n";
}

void TestChangeLeaderSuccess() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyCommandHandler handler(socialManager);

    handler.Invite(EntityId(1), EntityId(2));
    
    auto result = handler.ChangeLeader(EntityId(1), EntityId(2));
    assert(result.has_value());
    
    auto party = socialManager->GetParty();
    assert(party->GetMember(EntityId(1))->isLeader == false);
    assert(party->GetMember(EntityId(2))->isLeader == true);
    
    std::cout << "TestChangeLeaderSuccess passed.\n";
}

void TestChangeLeaderFailNotLeader() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyCommandHandler handler(socialManager);

    handler.Invite(EntityId(1), EntityId(2));
    handler.Invite(EntityId(1), EntityId(3));
    
    // 2 is not leader
    auto result = handler.ChangeLeader(EntityId(2), EntityId(3));
    assert(!result.has_value());
    assert(result.error() == CommandError::NotLeader);
    
    std::cout << "TestChangeLeaderFailNotLeader passed.\n";
}

void TestChangeLeaderFailPlayerNotFound() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyCommandHandler handler(socialManager);

    handler.Invite(EntityId(1), EntityId(2));
    
    // 3 is not in party
    auto result = handler.ChangeLeader(EntityId(1), EntityId(3));
    assert(!result.has_value());
    assert(result.error() == CommandError::PlayerNotFound);
    
    std::cout << "TestChangeLeaderFailPlayerNotFound passed.\n";
}

void TestChangeLeaderFailSelf() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyCommandHandler handler(socialManager);

    handler.Invite(EntityId(1), EntityId(2));
    
    auto result = handler.ChangeLeader(EntityId(1), EntityId(1));
    assert(!result.has_value());
    assert(result.error() == CommandError::InvalidTarget);
    
    std::cout << "TestChangeLeaderFailSelf passed.\n";
}

int main() {
    TestInviteSuccessNewParty();
    TestInviteSuccessExistingParty();
    TestInviteFailNotLeader();
    TestInviteFailAlreadyInParty();
    TestInviteFailSelf();
    TestLeaveSuccess();
    TestLeaveSuccessLeaderTransfer();
    TestLeaveSuccessEmptyPartyDestroyed();
    TestLeaveFailNotInParty();
    TestChangeLeaderSuccess();
    TestChangeLeaderFailNotLeader();
    TestChangeLeaderFailPlayerNotFound();
    TestChangeLeaderFailSelf();
    
    std::cout << "All tests passed!\n";
    return 0;
}
