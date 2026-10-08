#include <cassert>
#include <vector>
#include "../src/Client/Network/Handlers/ActorUpdateHandler.h"
#include "../src/UserInterface/Packet.h"

int main() {
    Client::Network::Handlers::ActorUpdateHandler handler;

    // Test 1: Buffer underflow
    std::vector<uint8_t> shortBuffer(10, 0);
    auto resUnderflow = handler.HandleActorUpdate(shortBuffer);
    assert(!resUnderflow.has_value());
    assert(resUnderflow.error() == EterBase::PacketError::BufferUnderflow);

    // Test 2: Valid packet
    TPacketGCCharacterUpdate packet{};
    packet.header = GC::CHARACTER_UPDATE;
    packet.length = sizeof(TPacketGCCharacterUpdate);
    packet.dwVID = 1337;
    packet.awPart[CHR_EQUIPPART_ARMOR] = 11299;
    packet.awPart[CHR_EQUIPPART_WEAPON] = 3159;
    packet.awPart[CHR_EQUIPPART_HAIR] = 73001;
    packet.bMovingSpeed = 150;
    packet.bAttackSpeed = 120;
    packet.dwGuildID = 42;

    std::span<const uint8_t> validSpan(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
    auto resValid = handler.HandleActorUpdate(validSpan);
    assert(resValid.has_value());
    assert(resValid->vid.Get() == 1337);
    assert(resValid->armorVnum == 11299);
    assert(resValid->weaponVnum == 3159);
    assert(resValid->hairVnum == 73001);
    assert(resValid->movingSpeed == 150);
    assert(resValid->attackSpeed == 120);
    assert(resValid->guildId == 42);

    // Test 3: Zero VID (malformed)
    packet.dwVID = 0;
    std::span<const uint8_t> invalidVidSpan(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
    auto resInvalidVid = handler.HandleActorUpdate(invalidVidSpan);
    assert(!resInvalidVid.has_value());
    assert(resInvalidVid.error() == EterBase::PacketError::MalformedPayload);

    return 0;
}
