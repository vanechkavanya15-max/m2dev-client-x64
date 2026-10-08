#include "dummy_headers.h"
#include "../src/Client/Network/Handlers/PartyHandler.h"
#include "../src/Client/Gameplay/SocialDomain.h"
#include "../src/UserInterface/Packet.h"
#include "../src/EterBase/StrongTypes.h"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace Client::Network::Handlers;
using namespace Client::Gameplay;

void TestHandlePartyInvite() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyHandler handler(socialManager);

    TPacketGCPartyInvite packet{};
    packet.header = GC::PARTY_INVITE; // Note: The packet header enum might not be exposed, just ensuring size
    packet.length = sizeof(TPacketGCPartyInvite);
    packet.leader_pid = 100;

    auto payload = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
    auto result = handler.HandlePartyInvite(payload);

    assert(result.has_value());
    std::cout << "TestHandlePartyInvite passed!\n";
}

void TestHandlePartyAdd() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyHandler handler(socialManager);

    TPacketGCPartyAdd packet{};
    packet.header = GC::PARTY_ADD;
    packet.length = sizeof(TPacketGCPartyAdd);
    packet.pid = 200;
    std::strncpy(packet.name, "TestUser", CHARACTER_NAME_MAX_LEN);

    auto payload = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
    auto result = handler.HandlePartyAdd(payload);

    assert(result.has_value());
    
    auto party = socialManager->GetParty();
    assert(party != nullptr);
    assert(party->GetMembers().size() == 1);
    
    auto memberOpt = party->GetMember(EterBase::EntityId(200));
    assert(memberOpt.has_value());
    assert(memberOpt->name == "TestUser");

    std::cout << "TestHandlePartyAdd passed!\n";
}

void TestHandlePartyUpdate() {
    auto socialManager = std::make_shared<SocialManager>();
    socialManager->CreateParty();
    socialManager->GetParty()->AddMember(PartyMember(EterBase::EntityId(200), "TestUser"));
    
    PartyHandler handler(socialManager);

    TPacketGCPartyUpdate packet{};
    packet.header = GC::PARTY_UPDATE;
    packet.length = sizeof(TPacketGCPartyUpdate);
    packet.pid = 200;
    packet.state = 1;
    packet.percent_hp = 50;

    auto payload = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
    auto result = handler.HandlePartyUpdate(payload);

    assert(result.has_value());

    auto memberOpt = socialManager->GetParty()->GetMember(EterBase::EntityId(200));
    assert(memberOpt.has_value());
    assert(memberOpt->hpPercentage == 50);

    std::cout << "TestHandlePartyUpdate passed!\n";
}

void TestHandlePartyRemove() {
    auto socialManager = std::make_shared<SocialManager>();
    socialManager->CreateParty();
    socialManager->GetParty()->AddMember(PartyMember(EterBase::EntityId(200), "TestUser"));
    
    PartyHandler handler(socialManager);

    TPacketGCPartyRemove packet{};
    packet.header = GC::PARTY_REMOVE;
    packet.length = sizeof(TPacketGCPartyRemove);
    packet.pid = 200;

    auto payload = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
    auto result = handler.HandlePartyRemove(payload);

    assert(result.has_value());
    
    // Party should be empty and leaving the party sets the social manager party to null
    assert(socialManager->GetParty() == nullptr);

    std::cout << "TestHandlePartyRemove passed!\n";
}

void TestBufferUnderflow() {
    auto socialManager = std::make_shared<SocialManager>();
    PartyHandler handler(socialManager);

    uint8_t smallBuffer[2] = {0x01, 0x02};
    auto payload = std::span<const uint8_t>(smallBuffer, sizeof(smallBuffer));

    auto result = handler.HandlePartyInvite(payload);
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::BufferUnderflow);
    
    std::cout << "TestBufferUnderflow passed!\n";
}

int main() {
    std::cout << "Running PartyHandler Tests...\n";
    TestHandlePartyInvite();
    TestHandlePartyAdd();
    TestHandlePartyUpdate();
    TestHandlePartyRemove();
    TestBufferUnderflow();
    std::cout << "All tests passed!\n";
    return 0;
}
