#include <cassert>
#include <iostream>
#include <vector>
#include "../src/Client/Network/CombatPacketCodec.h"

using namespace Network::CombatPacketCodec;

void Test_Attack_Codec()
{
    TPacketCGAttack packet{};
    packet.header = 0x02;
    packet.length = sizeof(TPacketCGAttack);
    packet.type = 1;
    packet.targetId = 12345;
    packet.crcMagicCubeProcPiece = 0xAA;
    packet.crcMagicCubeFilePiece = 0xBB;

    auto encodedResult = EncodeAttack(packet);
    assert(encodedResult.has_value());
    const auto& buffer = encodedResult.value();
    assert(buffer.size() == sizeof(TPacketCGAttack));

    auto decodedResult = DecodeAttack(buffer);
    assert(decodedResult.has_value());
    const auto& decoded = decodedResult.value();

    assert(decoded.header == packet.header);
    assert(decoded.type == packet.type);
    assert(decoded.targetId == packet.targetId);
}

void Test_Shoot_Codec()
{
    TPacketCGShoot packet{};
    packet.header = 0x03;
    packet.length = sizeof(TPacketCGShoot);
    packet.bType = 2;

    auto encoded = EncodeShoot(packet);
    assert(encoded.has_value());

    auto decoded = DecodeShoot(encoded.value());
    assert(decoded.has_value());
    assert(decoded.value().bType == 2);
}

void Test_DamageInfo_Codec_Valid()
{
    TPacketGCDamageInfo packet{};
    packet.header = 0x04;
    packet.length = sizeof(TPacketGCDamageInfo);
    packet.targetId = 54321;
    packet.damage = 999;
    packet.flag = (1 << 5); // DAMAGE_CRITICAL

    auto encoded = EncodeDamageInfo(packet);
    assert(encoded.has_value());

    auto decoded = DecodeDamageInfo(encoded.value());
    assert(decoded.has_value());
    assert(decoded.value().damage == 999);
    assert(decoded.value().flag == (1 << 5));
}

void Test_DamageInfo_Codec_InvalidFlag()
{
    TPacketGCDamageInfo packet{};
    packet.header = 0x04;
    packet.length = sizeof(TPacketGCDamageInfo);
    packet.targetId = 54321;
    packet.damage = 999;
    // (1<<6) is invalid, max is (1<<5)
    packet.flag = (1 << 6); 

    std::vector<uint8_t> badBuffer(sizeof(TPacketGCDamageInfo));
    std::memcpy(badBuffer.data(), &packet, sizeof(TPacketGCDamageInfo));

    auto decoded = DecodeDamageInfo(badBuffer);
    assert(!decoded.has_value());
    assert(decoded.error() == EterBase::PacketError::MalformedPayload);
}

void Test_Dead_Codec()
{
    TPacketGCDead packet{};
    packet.header = 0x05;
    packet.length = sizeof(TPacketGCDead);
    packet.vid = 11111;

    auto encoded = EncodeDead(packet);
    assert(encoded.has_value());

    auto decoded = DecodeDead(encoded.value());
    assert(decoded.has_value());
    assert(decoded.value().vid == 11111);
}

void Test_BufferUnderflow()
{
    std::vector<uint8_t> shortBuffer = {0x02, 0x00, 0x00}; // Too short for any packet

    auto res1 = DecodeAttack(shortBuffer);
    assert(!res1.has_value() && res1.error() == EterBase::PacketError::BufferUnderflow);

    auto res2 = DecodeShoot(shortBuffer);
    assert(!res2.has_value() && res2.error() == EterBase::PacketError::BufferUnderflow);

    auto res3 = DecodeDamageInfo(shortBuffer);
    assert(!res3.has_value() && res3.error() == EterBase::PacketError::BufferUnderflow);

    auto res4 = DecodeDead(shortBuffer);
    assert(!res4.has_value() && res4.error() == EterBase::PacketError::BufferUnderflow);
}

int main()
{
    Test_Attack_Codec();
    Test_Shoot_Codec();
    Test_DamageInfo_Codec_Valid();
    Test_DamageInfo_Codec_InvalidFlag();
    Test_Dead_Codec();
    Test_BufferUnderflow();

    std::cout << "All CombatPacketCodec tests passed successfully!" << std::endl;
    return 0;
}
