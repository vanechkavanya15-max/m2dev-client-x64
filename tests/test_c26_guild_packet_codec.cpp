#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/Client/Network/GuildPacketCodec.h"
#include <cstring>
#include <vector>

using namespace Client::Network;

TEST_CASE("GuildPacketCodec: Header Decode") {
    SUBCASE("Valid") {
        TPacketGCGuild mock;
        mock.header = 0x0720;
        mock.length = sizeof(mock);
        mock.subheader = 0x01;
        
        std::vector<uint8_t> buffer(sizeof(mock));
        std::memcpy(buffer.data(), &mock, sizeof(mock));
        
        auto res = DecodeGuildHeader(buffer);
        REQUIRE(res.has_value());
        CHECK(res->header == mock.header);
        CHECK(res->length == mock.length);
        CHECK(res->subheader == mock.subheader);
    }
    SUBCASE("Underflow") {
        std::vector<uint8_t> buffer(sizeof(TPacketGCGuild) - 1);
        auto res = DecodeGuildHeader(buffer);
        REQUIRE(!res.has_value());
        CHECK(res.error() == EterBase::PacketError::BufferUnderflow);
    }
}

TEST_CASE("GuildPacketCodec: SubInfo Decode") {
    SUBCASE("Valid") {
        TPacketGCGuildSubInfo mock{};
        mock.guild_id = 12345;
        mock.master_pid = 98765;
        mock.member_count = 10;
        mock.level = 20;
        std::strncpy(mock.name, "TestGuild", sizeof(mock.name));
        
        std::vector<uint8_t> buffer(sizeof(mock));
        std::memcpy(buffer.data(), &mock, sizeof(mock));
        
        auto res = DecodeGuildSubInfo(buffer);
        REQUIRE(res.has_value());
        CHECK(res->guild_id == mock.guild_id);
        CHECK(res->master_pid == mock.master_pid);
        CHECK(std::strcmp(res->name, "TestGuild") == 0);
    }
    SUBCASE("Underflow") {
        std::vector<uint8_t> buffer(sizeof(TPacketGCGuildSubInfo) - 1);
        auto res = DecodeGuildSubInfo(buffer);
        REQUIRE(!res.has_value());
        CHECK(res.error() == EterBase::PacketError::BufferUnderflow);
    }
}

TEST_CASE("GuildPacketCodec: SubMember Decode") {
    SUBCASE("Valid") {
        TPacketGCGuildSubMember mock{};
        mock.pid = 999;
        mock.byGrade = 1;
        mock.byLevel = 55;
        
        std::vector<uint8_t> buffer(sizeof(mock));
        std::memcpy(buffer.data(), &mock, sizeof(mock));
        
        auto res = DecodeGuildSubMember(buffer);
        REQUIRE(res.has_value());
        CHECK(res->pid == mock.pid);
        CHECK(res->byGrade == mock.byGrade);
        CHECK(res->byLevel == mock.byLevel);
    }
    SUBCASE("Underflow") {
        std::vector<uint8_t> buffer(sizeof(TPacketGCGuildSubMember) - 1);
        auto res = DecodeGuildSubMember(buffer);
        REQUIRE(!res.has_value());
        CHECK(res.error() == EterBase::PacketError::BufferUnderflow);
    }
}

TEST_CASE("GuildPacketCodec: SubWar Decode") {
    SUBCASE("Valid") {
        TPacketGCGuildSubWar mock{};
        mock.dwGuildSelf = 101;
        mock.dwGuildOpp = 102;
        mock.bType = 1;
        mock.bWarState = 2; // GUILD_WAR_REFUSE, etc
        
        std::vector<uint8_t> buffer(sizeof(mock));
        std::memcpy(buffer.data(), &mock, sizeof(mock));
        
        auto res = DecodeGuildSubWar(buffer);
        REQUIRE(res.has_value());
        CHECK(res->dwGuildSelf == mock.dwGuildSelf);
        CHECK(res->dwGuildOpp == mock.dwGuildOpp);
        CHECK(res->bType == mock.bType);
        CHECK(res->bWarState == mock.bWarState);
    }
    SUBCASE("Underflow") {
        std::vector<uint8_t> buffer(sizeof(TPacketGCGuildSubWar) - 1);
        auto res = DecodeGuildSubWar(buffer);
        REQUIRE(!res.has_value());
        CHECK(res.error() == EterBase::PacketError::BufferUnderflow);
    }
}

TEST_CASE("GuildPacketCodec: AddMember Encode") {
    uint32_t targetVid = 5555;
    auto buffer = EncodeGuildAddMember(targetVid);
    
    REQUIRE(buffer.size() == sizeof(TPacketCGGuild) + sizeof(uint32_t));
    
    TPacketCGGuild header{};
    std::memcpy(&header, buffer.data(), sizeof(header));
    
    CHECK(header.header == CG::GUILD);
    CHECK(header.length == buffer.size());
    CHECK(header.bySubHeader == GuildSub::CG::ADD_MEMBER);
    
    uint32_t decodedVid;
    std::memcpy(&decodedVid, buffer.data() + sizeof(header), sizeof(uint32_t));
    CHECK(decodedVid == targetVid);
}

TEST_CASE("GuildPacketCodec: RemoveMember Encode") {
    uint32_t targetPid = 7777;
    auto buffer = EncodeGuildRemoveMember(targetPid);
    
    REQUIRE(buffer.size() == sizeof(TPacketCGGuild) + sizeof(uint32_t));
    
    TPacketCGGuild header{};
    std::memcpy(&header, buffer.data(), sizeof(header));
    
    CHECK(header.header == CG::GUILD);
    CHECK(header.length == buffer.size());
    CHECK(header.bySubHeader == GuildSub::CG::REMOVE_MEMBER);
    
    uint32_t decodedPid;
    std::memcpy(&decodedPid, buffer.data() + sizeof(header), sizeof(uint32_t));
    CHECK(decodedPid == targetPid);
}
