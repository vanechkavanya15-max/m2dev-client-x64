#include <iostream>
#include <cassert>
#include <vector>
#include <cstring>
#include "../src/Client/Network/Handlers/SelectPacketHandler.h"
#include "../src/UserInterface/Packet.h"
#include "../src/UserInterface/Core/EventBus.h"

using namespace Client::Network::Handlers;

bool updatedEventFired = false;
CharacterSlotData lastUpdatedData;

bool deletedEventFired = false;
uint8_t lastDeletedSlot = 255;

void OnCharacterSlotUpdated(const CharacterSlotUpdatedEvent& event) {
    updatedEventFired = true;
    lastUpdatedData = event.slotData;
}

void OnCharacterSlotDeleted(const CharacterSlotDeletedEvent& event) {
    deletedEventFired = true;
    lastDeletedSlot = event.slotIndex;
}

void ResetEvents() {
    updatedEventFired = false;
    deletedEventFired = false;
    lastDeletedSlot = 255;
    lastUpdatedData = CharacterSlotData{};
}

void TestHandleLoginSuccess4() {
    ResetEvents();

    TPacketGCLoginSuccess4 packet{};
    packet.header = 0; // Not checked by payload size
    packet.length = sizeof(packet);

    packet.akSimplePlayerInformation[0].dwID = 1001;
    std::strncpy(packet.akSimplePlayerInformation[0].szName, "Warrior", CHARACTER_NAME_MAX_LEN);
    packet.akSimplePlayerInformation[0].byJob = 0;
    packet.akSimplePlayerInformation[0].byLevel = 55;
    packet.guild_id[0] = 50;
    std::strncpy(packet.guild_name[0], "HeroGuild", GUILD_NAME_MAX_LEN);

    packet.akSimplePlayerInformation[1].dwID = 1002;
    std::strncpy(packet.akSimplePlayerInformation[1].szName, "Ninja", CHARACTER_NAME_MAX_LEN);
    packet.akSimplePlayerInformation[1].byJob = 1;
    packet.akSimplePlayerInformation[1].byLevel = 30;
    packet.guild_id[1] = 0;

    std::vector<uint8_t> payload(sizeof(packet));
    std::memcpy(payload.data(), &packet, sizeof(packet));

    auto result = SelectPacketHandler::HandleLoginSuccess4(payload);
    assert(result.has_value());

    // Event handlers are synchronous, so updatedEventFired should have been called twice.
    // The last updated data will be the second character.
    assert(updatedEventFired);
    assert(lastUpdatedData.id == 1002);
    assert(lastUpdatedData.name == "Ninja");
    assert(lastUpdatedData.slotIndex == 1);
    
    std::cout << "TestHandleLoginSuccess4 passed." << std::endl;
}

void TestHandleCreateSuccess() {
    ResetEvents();

    TPacketGCPlayerCreateSuccess packet{};
    packet.bAccountCharacterSlot = 2;
    packet.kSimplePlayerInfomation.dwID = 1003;
    std::strncpy(packet.kSimplePlayerInfomation.szName, "Sura", CHARACTER_NAME_MAX_LEN);
    packet.kSimplePlayerInfomation.byJob = 2;
    packet.kSimplePlayerInfomation.byLevel = 1;

    std::vector<uint8_t> payload(sizeof(packet));
    std::memcpy(payload.data(), &packet, sizeof(packet));

    auto result = SelectPacketHandler::HandleCreateSuccess(payload);
    assert(result.has_value());

    assert(updatedEventFired);
    assert(lastUpdatedData.id == 1003);
    assert(lastUpdatedData.name == "Sura");
    assert(lastUpdatedData.slotIndex == 2);
    assert(lastUpdatedData.guildId == 0); // Guild ID should be 0 on creation

    std::cout << "TestHandleCreateSuccess passed." << std::endl;
}

void TestHandleDeleteSuccess() {
    ResetEvents();

    TPacketGCDestroyCharacterSuccess packet{};
    packet.account_index = 1;

    std::vector<uint8_t> payload(sizeof(packet));
    std::memcpy(payload.data(), &packet, sizeof(packet));

    auto result = SelectPacketHandler::HandleDeleteSuccess(payload);
    assert(result.has_value());

    assert(deletedEventFired);
    assert(lastDeletedSlot == 1);

    std::cout << "TestHandleDeleteSuccess passed." << std::endl;
}

int main() {
    auto subId1 = UserInterface::Core::EventBus::Instance().Subscribe<CharacterSlotUpdatedEvent>(OnCharacterSlotUpdated);
    auto subId2 = UserInterface::Core::EventBus::Instance().Subscribe<CharacterSlotDeletedEvent>(OnCharacterSlotDeleted);

    TestHandleLoginSuccess4();
    TestHandleCreateSuccess();
    TestHandleDeleteSuccess();

    UserInterface::Core::EventBus::Instance().Unsubscribe<CharacterSlotUpdatedEvent>(subId1);
    UserInterface::Core::EventBus::Instance().Unsubscribe<CharacterSlotDeletedEvent>(subId2);

    std::cout << "All tests passed." << std::endl;
    return 0;
}
