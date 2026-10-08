#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../vendor/doctest/doctest.h"

// Set macro to mock StdAfx headers internally for tests
#define ETERBASE_STDAFX_H

#include "../src/Client/Network/SkillPacketCodec.h"

using namespace Client::Network;

TEST_CASE("SkillPacketCodec::DecodeSkillLevel")
{
    std::vector<uint8_t> buffer(sizeof(TPacketGCSkillLevel), 0);
    TPacketGCSkillLevel* p = reinterpret_cast<TPacketGCSkillLevel*>(buffer.data());
    p->header = CG::SKILL_LEVEL;
    p->length = sizeof(TPacketGCSkillLevel);
    
    auto result = SkillPacketCodec::DecodeSkillLevel(buffer);
    REQUIRE(result.has_value());
    CHECK(result->header == CG::SKILL_LEVEL);
    CHECK(result->length == sizeof(TPacketGCSkillLevel));
    
    std::vector<uint8_t> small_buffer(1);
    auto small_result = SkillPacketCodec::DecodeSkillLevel(small_buffer);
    REQUIRE(!small_result.has_value());
}

TEST_CASE("SkillPacketCodec::DecodeSkillLevelNew")
{
    std::vector<uint8_t> buffer(sizeof(TPacketGCSkillLevelNew), 0);
    TPacketGCSkillLevelNew* p = reinterpret_cast<TPacketGCSkillLevelNew*>(buffer.data());
    p->header = CG::SKILL_LEVEL_NEW;
    p->length = sizeof(TPacketGCSkillLevelNew);
    
    // In our isolated test with the actual packet header, time_t on 64-bit Linux is 8 bytes, struct aligns to 8 bytes, making it exactly 2554 on x64.
    if (sizeof(TPacketGCSkillLevelNew) == 2554)
    {
        auto result = SkillPacketCodec::DecodeSkillLevelNew(buffer);
        REQUIRE(result.has_value());
        CHECK(result->header == CG::SKILL_LEVEL_NEW);
    }
    else
    {
        auto result = SkillPacketCodec::DecodeSkillLevelNew(buffer);
        REQUIRE(!result.has_value());
    }
}

TEST_CASE("SkillPacketCodec::DecodeSkillCooltimeEnd")
{
    std::vector<uint8_t> buffer(sizeof(TPacketGCSkillCoolTimeEnd), 0);
    TPacketGCSkillCoolTimeEnd* p = reinterpret_cast<TPacketGCSkillCoolTimeEnd*>(buffer.data());
    p->header = CG::SKILL_COOLTIME_END;
    p->length = sizeof(TPacketGCSkillCoolTimeEnd);
    p->bSkill = 1;
    
    auto result = SkillPacketCodec::DecodeSkillCooltimeEnd(buffer);
    REQUIRE(result.has_value());
    CHECK(result->header == CG::SKILL_COOLTIME_END);
    CHECK(result->bSkill == 1);
}

TEST_CASE("SkillPacketCodec::EncodeUseSkill")
{
    auto buffer = SkillPacketCodec::EncodeUseSkill(100, 200);
    REQUIRE(buffer.size() == sizeof(TPacketCGUseSkill));
    
    TPacketCGUseSkill* packet = reinterpret_cast<TPacketCGUseSkill*>(buffer.data());
    CHECK(packet->header == CG::USE_SKILL);
    CHECK(packet->length == sizeof(TPacketCGUseSkill));
    CHECK(packet->dwVnum == 100);
    CHECK(packet->dwTargetVID == 200);
}
