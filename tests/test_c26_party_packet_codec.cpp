#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#define ETERBASE_STDAFX_H

#include <cstdint>
#include <cstring>

// Mocks to avoid including legacy headers
#define CHARACTER_NAME_MAX_LEN 24
#define PARTY_AFFECT_SLOT_MAX_NUM 7

typedef struct packet_party_invite
{
    uint16_t	header;
    uint16_t	length;
    uint32_t leader_pid;
} TPacketGCPartyInvite;

typedef struct packet_party_add
{
    uint16_t	header;
    uint16_t	length;
    uint32_t pid;
    char name[CHARACTER_NAME_MAX_LEN+1];
} TPacketGCPartyAdd;

typedef struct packet_party_update
{
    uint16_t	header;
    uint16_t	length;
    uint32_t pid;
    uint8_t state;
    uint8_t percent_hp;
    int16_t affects[PARTY_AFFECT_SLOT_MAX_NUM];
} TPacketGCPartyUpdate;

typedef struct packet_party_remove
{
    uint16_t	header;
    uint16_t	length;
    uint32_t pid;
} TPacketGCPartyRemove;

typedef struct paryt_parameter
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        bDistributeMode;
} TPacketGCPartyParameter;

typedef struct command_party_invite_answer
{
    uint16_t	header;
    uint16_t	length;
    uint32_t leader_pid;
    uint8_t accept;
} TPacketCGPartyInviteAnswer;

#include "../src/Client/Network/PartyPacketCodec.cpp"

using namespace Client::Network;

TEST_CASE("PartyPacketCodec - DecodePartyInvite") {
    TPacketGCPartyInvite pkt;
    pkt.header = 0x0710;
    pkt.length = sizeof(pkt);
    pkt.leader_pid = 12345;

    std::vector<uint8_t> buf(sizeof(pkt));
    std::memcpy(buf.data(), &pkt, sizeof(pkt));

    auto res = PartyPacketCodec::DecodePartyInvite(buf);
    REQUIRE(res.has_value());
    CHECK(res->header == 0x0710);
    CHECK(res->leader_pid == 12345);

    auto bad_res = PartyPacketCodec::DecodePartyInvite(std::span(buf).first(sizeof(pkt) - 1));
    REQUIRE(!bad_res.has_value());
}

TEST_CASE("PartyPacketCodec - DecodePartyAdd") {
    TPacketGCPartyAdd pkt;
    pkt.header = 0x0711;
    pkt.length = sizeof(pkt);
    pkt.pid = 54321;
    std::strcpy(pkt.name, "TestUser");

    std::vector<uint8_t> buf(sizeof(pkt));
    std::memcpy(buf.data(), &pkt, sizeof(pkt));

    auto res = PartyPacketCodec::DecodePartyAdd(buf);
    REQUIRE(res.has_value());
    CHECK(res->header == 0x0711);
    CHECK(res->pid == 54321);
    CHECK(std::string(res->name) == "TestUser");

    auto bad_res = PartyPacketCodec::DecodePartyAdd(std::span(buf).first(sizeof(pkt) - 1));
    REQUIRE(!bad_res.has_value());
}

TEST_CASE("PartyPacketCodec - DecodePartyUpdate") {
    TPacketGCPartyUpdate pkt;
    pkt.header = 0x0712;
    pkt.length = sizeof(pkt);
    pkt.pid = 999;
    pkt.state = 2;
    pkt.percent_hp = 100;
    
    std::vector<uint8_t> buf(sizeof(pkt));
    std::memcpy(buf.data(), &pkt, sizeof(pkt));

    auto res = PartyPacketCodec::DecodePartyUpdate(buf);
    REQUIRE(res.has_value());
    CHECK(res->header == 0x0712);
    CHECK(res->pid == 999);
    CHECK(res->state == 2);
    CHECK(res->percent_hp == 100);

    auto bad_res = PartyPacketCodec::DecodePartyUpdate(std::span(buf).first(sizeof(pkt) - 1));
    REQUIRE(!bad_res.has_value());
}

TEST_CASE("PartyPacketCodec - DecodePartyRemove") {
    TPacketGCPartyRemove pkt;
    pkt.header = 0x0713;
    pkt.length = sizeof(pkt);
    pkt.pid = 111;

    std::vector<uint8_t> buf(sizeof(pkt));
    std::memcpy(buf.data(), &pkt, sizeof(pkt));

    auto res = PartyPacketCodec::DecodePartyRemove(buf);
    REQUIRE(res.has_value());
    CHECK(res->header == 0x0713);
    CHECK(res->pid == 111);

    auto bad_res = PartyPacketCodec::DecodePartyRemove(std::span(buf).first(sizeof(pkt) - 1));
    REQUIRE(!bad_res.has_value());
}

TEST_CASE("PartyPacketCodec - DecodePartyParameter") {
    TPacketGCPartyParameter pkt;
    pkt.header = 0x0716;
    pkt.length = sizeof(pkt);
    pkt.bDistributeMode = 1;

    std::vector<uint8_t> buf(sizeof(pkt));
    std::memcpy(buf.data(), &pkt, sizeof(pkt));

    auto res = PartyPacketCodec::DecodePartyParameter(buf);
    REQUIRE(res.has_value());
    CHECK(res->header == 0x0716);
    CHECK(res->bDistributeMode == 1);

    auto bad_res = PartyPacketCodec::DecodePartyParameter(std::span(buf).first(sizeof(pkt) - 1));
    REQUIRE(!bad_res.has_value());
}

TEST_CASE("PartyPacketCodec - EncodePartyInviteAnswer") {
    auto buf = PartyPacketCodec::EncodePartyInviteAnswer(777, 1);
    
    REQUIRE(buf.size() == sizeof(TPacketCGPartyInviteAnswer));
    
    TPacketCGPartyInviteAnswer pkt;
    std::memcpy(&pkt, buf.data(), sizeof(pkt));
    
    CHECK(pkt.header == 0x0702); // PARTY_INVITE_ANSWER
    CHECK(pkt.length == sizeof(pkt));
    CHECK(pkt.leader_pid == 777);
    CHECK(pkt.accept == 1);
}
