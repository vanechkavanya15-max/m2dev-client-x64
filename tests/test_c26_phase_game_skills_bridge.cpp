#define TEST_MODE_DISABLE_STDAFX
#include "doctest.h"
#include <expected>
#include <span>
#include <cstdint>
#include <vector>

// Dummy types for test
#define DWORD uint32_t
#define BYTE uint8_t
#define BOOL int

namespace EterBase {
    enum class PacketError { BufferUnderflow };
    template <typename T>
    using PacketResult = std::expected<T, PacketError>;
}

#define SKILL_MAX_NUM 255

typedef struct packet_skill_level
{
    uint16_t	header;
    uint16_t	length;
    uint8_t        abSkillLevels[SKILL_MAX_NUM];
} TPacketGCSkillLevel;

typedef struct SPlayerSkill
{
	uint8_t bMasterType;
	uint8_t bLevel;
	uint32_t tNextRead;
} TPlayerSkill;

typedef struct packet_skill_level_new
{
	uint16_t	header;
	uint16_t	length;
	TPlayerSkill skills[SKILL_MAX_NUM];
} TPacketGCSkillLevelNew;

typedef struct packet_skill_cooltime_end
{
	uint16_t	header;
	uint16_t	length;
	uint8_t		bSkill;
} TPacketGCSkillCoolTimeEnd;

namespace Client::Network {
    struct SkillPacketCodec {
        static EterBase::PacketResult<TPacketGCSkillLevel> DecodeSkillLevel(std::span<const uint8_t> buffer) {
            if (buffer.size() < sizeof(TPacketGCSkillLevel)) return std::unexpected(EterBase::PacketError::BufferUnderflow);
            return *reinterpret_cast<const TPacketGCSkillLevel*>(buffer.data());
        }
        static EterBase::PacketResult<TPacketGCSkillLevelNew> DecodeSkillLevelNew(std::span<const uint8_t> buffer) {
            if (buffer.size() < sizeof(TPacketGCSkillLevelNew)) return std::unexpected(EterBase::PacketError::BufferUnderflow);
            return *reinterpret_cast<const TPacketGCSkillLevelNew*>(buffer.data());
        }
        static EterBase::PacketResult<TPacketGCSkillCoolTimeEnd> DecodeSkillCooltimeEnd(std::span<const uint8_t> buffer) {
            if (buffer.size() < sizeof(TPacketGCSkillCoolTimeEnd)) return std::unexpected(EterBase::PacketError::BufferUnderflow);
            return *reinterpret_cast<const TPacketGCSkillCoolTimeEnd*>(buffer.data());
        }
    };
}

class CPythonNetworkStream {
public:
    bool refreshSkillWindowCalled = false;
    bool refreshStatusCalled = false;

    void __RefreshSkillWindow() { refreshSkillWindowCalled = true; }
    void __RefreshStatus() { refreshStatusCalled = true; }
};

struct SkillSetData {
    int skillIndex;
    uint32_t bMasterType;
    uint32_t bLevel;
};

class CPythonPlayer {
public:
    static CPythonPlayer& Instance() {
        static CPythonPlayer instance;
        return instance;
    }

    std::vector<std::pair<DWORD, int>> setSkillLevels;
    std::vector<std::pair<int, int>> setSkills;
    std::vector<SkillSetData> setSkillLevel_s;
    std::vector<uint8_t> endedCoolTimes;

    bool GetSkillSlotIndex(int skillIndex, DWORD* slotIndex) {
        *slotIndex = skillIndex; // Mock simple mapping 1:1
        return true;
    }

    void SetSkillLevel(DWORD dwSlotIndex, int level) {
        setSkillLevels.push_back({dwSlotIndex, level});
    }

    void SetSkill(int skillIndex, int level) {
        setSkills.push_back({skillIndex, level});
    }

    void SetSkillLevel_(int index, uint32_t masterType, uint32_t level) {
        setSkillLevel_s.push_back({index, masterType, level});
    }

    void EndSkillCoolTime(uint8_t bSkill) {
        endedCoolTimes.push_back(bSkill);
    }
    
    void clearMocks() {
        setSkillLevels.clear();
        setSkills.clear();
        setSkillLevel_s.clear();
        endedCoolTimes.clear();
    }
};

// Include the source directly for isolated test
#include "../src/UserInterface/PythonNetworkStreamPhaseGameSkills.cpp"

TEST_CASE("HandleSkillLevel") {
    CPythonPlayer::Instance().clearMocks();
    CPythonNetworkStream stream;
    TPacketGCSkillLevel pack{};
    pack.abSkillLevels[5] = 10;
    
    bool result = PhaseGameSkillsBridge::HandleSkillLevel(&stream, pack);
    
    CHECK(result == true);
    CHECK(stream.refreshSkillWindowCalled == true);
    CHECK(stream.refreshStatusCalled == true);
    CHECK(CPythonPlayer::Instance().setSkillLevels.size() == SKILL_MAX_NUM);
    CHECK(CPythonPlayer::Instance().setSkillLevels[5].first == 5);
    CHECK(CPythonPlayer::Instance().setSkillLevels[5].second == 10);
}

TEST_CASE("HandleSkillLevelNew") {
    CPythonPlayer::Instance().clearMocks();
    CPythonNetworkStream stream;
    TPacketGCSkillLevelNew pack{};
    pack.skills[114].bLevel = 5;
    pack.skills[114].bMasterType = 1;
    pack.skills[118].bLevel = 6;
    pack.skills[118].bMasterType = 2;
    pack.skills[10].bLevel = 7;
    pack.skills[10].bMasterType = 0;
    
    bool result = PhaseGameSkillsBridge::HandleSkillLevelNew(&stream, pack);
    
    CHECK(result == true);
    CHECK(stream.refreshSkillWindowCalled == true);
    CHECK(stream.refreshStatusCalled == true);
    CHECK(CPythonPlayer::Instance().setSkills.size() == 4); // two resets + two conditionally set
    CHECK(CPythonPlayer::Instance().setSkills[0] == std::make_pair(7, 0));
    CHECK(CPythonPlayer::Instance().setSkills[1] == std::make_pair(8, 0));
    CHECK(CPythonPlayer::Instance().setSkills[2] == std::make_pair(7, 114));
    CHECK(CPythonPlayer::Instance().setSkills[3] == std::make_pair(8, 118));
    
    CHECK(CPythonPlayer::Instance().setSkillLevel_s.size() == SKILL_MAX_NUM);
    CHECK(CPythonPlayer::Instance().setSkillLevel_s[114].skillIndex == 114);
    CHECK(CPythonPlayer::Instance().setSkillLevel_s[114].bMasterType == 1);
    CHECK(CPythonPlayer::Instance().setSkillLevel_s[114].bLevel == 5);
}

TEST_CASE("HandleSkillCooltimeEnd") {
    CPythonPlayer::Instance().clearMocks();
    CPythonNetworkStream stream;
    TPacketGCSkillCoolTimeEnd pack{};
    pack.bSkill = 42;
    
    bool result = PhaseGameSkillsBridge::HandleSkillCooltimeEnd(&stream, pack);
    
    CHECK(result == true);
    CHECK(CPythonPlayer::Instance().endedCoolTimes.size() == 1);
    CHECK(CPythonPlayer::Instance().endedCoolTimes[0] == 42);
}
