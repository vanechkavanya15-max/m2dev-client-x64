#include "../src/Client/Network/ActorPacketCodec.h"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace Client::Network;

void TestDecodeCharacterAdd() {
    TPacketGCCharacterAdd packet{};
    packet.header = 1;
    packet.dwVID = 1337;
    packet.wRaceNum = 1000;
    packet.x = 10.5f;
    packet.y = 20.5f;
    packet.z = 30.5f;
    packet.angle = 180.0f;
    packet.bType = 2;
    packet.bStateFlag = 1;
    packet.bAffectFlag[0] = 0xFF;
    packet.bAffectFlag[1] = 0xAA;

    auto result = ActorPacketCodec::DecodeCharacterAdd(packet);
    assert(result.has_value());
    
    auto data = result.value();
    assert(data.vid.value() == 1337);
    assert(data.race.value() == 1000);
    assert(data.x == 10.5f);
    assert(data.y == 20.5f);
    assert(data.z == 30.5f);
    assert(data.angle == 180.0f);
    assert(data.type == 2);
    assert(data.stateFlag == 1);
    assert(data.affectFlag[0] == 0xFF);
    assert(data.affectFlag[1] == 0xAA);
    assert(data.hasAdditionalInfo == false);
    
    std::cout << "TestDecodeCharacterAdd passed!\n";
}

void TestDecodeCharacterAdditionalInfo() {
    ActorSpawnData existingData{};
    existingData.vid = EntityVid(1337);

    TPacketGCCharacterAdditionalInfo packet{};
    packet.header = 2;
    packet.dwVID = 1337;
    std::strncpy(packet.name, "TestActorName", CHARACTER_NAME_MAX_LEN);
    packet.name[CHARACTER_NAME_MAX_LEN] = '\0';
    packet.awPart[0] = 10;
    packet.awPart[1] = 20;
    packet.awPart[2] = 30;
    packet.awPart[3] = 40;
    packet.bEmpire = 1;
    packet.dwGuildID = 55;
    packet.dwLevel = 99;
    packet.sAlignment = 1000;
    packet.bPKMode = 3;
    packet.dwMountVnum = 20110;

    auto result = ActorPacketCodec::DecodeCharacterAdditionalInfo(packet, existingData);
    assert(result.has_value());

    auto data = result.value();
    assert(data.vid.value() == 1337);
    assert(data.name == "TestActorName");
    assert(data.parts[0] == 10);
    assert(data.parts[3] == 40);
    assert(data.empire == 1);
    assert(data.guildId.value() == 55);
    assert(data.level.value() == 99);
    assert(data.alignment == 1000);
    assert(data.pkMode == 3);
    assert(data.mountVnum.value() == 20110);
    assert(data.hasAdditionalInfo == true);
    
    std::cout << "TestDecodeCharacterAdditionalInfo passed!\n";
}

void TestDecodeCharacterUpdate() {
    ActorSpawnData existingData{};
    existingData.vid = EntityVid(1337);
    
    TPacketGCCharacterUpdate packet{};
    packet.header = 3;
    packet.dwVID = 1337;
    packet.awPart[0] = 11;
    packet.bStateFlag = 5;
    packet.bAffectFlag[0] = 0x11;
    packet.bAffectFlag[1] = 0x22;
    packet.dwGuildID = 56;
    packet.sAlignment = -100;
    packet.bPKMode = 2;
    packet.dwMountVnum = 20111;

    auto result = ActorPacketCodec::DecodeCharacterUpdate(packet, existingData);
    assert(result.has_value());

    auto data = result.value();
    assert(data.vid.value() == 1337);
    assert(data.parts[0] == 11);
    assert(data.stateFlag == 5);
    assert(data.affectFlag[0] == 0x11);
    assert(data.affectFlag[1] == 0x22);
    assert(data.guildId.value() == 56);
    assert(data.alignment == -100);
    assert(data.pkMode == 2);
    assert(data.mountVnum.value() == 20111);

    std::cout << "TestDecodeCharacterUpdate passed!\n";
}

void TestDecodeCharacterDelete() {
    TPacketGCCharacterDelete packet{};
    packet.header = 4;
    packet.dwVID = 1337;

    auto result = ActorPacketCodec::DecodeCharacterDelete(packet);
    assert(result.has_value());
    assert(result.value().value() == 1337);

    std::cout << "TestDecodeCharacterDelete passed!\n";
}

void TestSequenceMismatch() {
    ActorSpawnData existingData{};
    existingData.vid = EntityVid(100);

    TPacketGCCharacterUpdate packet{};
    packet.dwVID = 200; // Mismatch

    auto result = ActorPacketCodec::DecodeCharacterUpdate(packet, existingData);
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::SequenceMismatch);

    std::cout << "TestSequenceMismatch passed!\n";
}

int main() {
    std::cout << "Running ActorPacketCodec Tests...\n";
    TestDecodeCharacterAdd();
    TestDecodeCharacterAdditionalInfo();
    TestDecodeCharacterUpdate();
    TestDecodeCharacterDelete();
    TestSequenceMismatch();
    std::cout << "All tests passed!\n";
    return 0;
}
