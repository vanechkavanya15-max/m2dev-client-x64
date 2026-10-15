#include "doctest.h"

// Define legacy macros for mock dependencies
#define CHARACTER_NAME_MAX_LEN 24
#define GUILD_NAME_MAX_LEN 12
#define TraceError(...) 

#include <cstdint>
#include <vector>
#include <cstring>
#include <string>
#include <span>
#include <unordered_map>

// Mock expected structures
#pragma pack(push, 1)

typedef struct packet_guild
{
	uint8_t header;
	uint16_t size;
	uint8_t subheader;
} TPacketGCGuild;

typedef struct packet_guild_info
{
	uint8_t header;
	uint16_t size;
	uint8_t subheader;
	uint32_t guild_id;
	uint32_t master_pid;
	uint32_t level;
	uint32_t exp;
	uint32_t member_count;
	uint32_t max_member_count;
	uint32_t gold;
	uint8_t hasLand;
	char name[GUILD_NAME_MAX_LEN+1];
} TPacketGCGuildInfo;

typedef struct packet_guild_sub_member
{
	uint32_t pid;
	uint8_t byGrade;
	uint8_t byIsGeneral;
	uint8_t byJob;
	uint8_t byLevel;
	uint32_t dwOffer;
	uint8_t byNameFlag;
} TPacketGCGuildSubMember;

typedef struct packet_guild_war
{
	uint32_t dwGuildSelf;
	uint32_t dwGuildOpp;
	uint8_t bType;
	uint8_t bWarState;
} TPacketGCGuildWar;

#pragma pack(pop)

namespace GuildSub::GC {
	enum {
		LOGIN = 1,
		LOGOUT,
		REMOVE,
		LIST,
		GRADE,
		GRADE_NAME,
		GRADE_AUTH,
		INFO,
		COMMENTS,
		CHANGE_EXP,
		CHANGE_MEMBER_GRADE,
		SKILL_INFO,
		CHANGE_MEMBER_GENERAL,
		GUILD_INVITE,
		WAR,
		GUILD_NAME,
		GUILD_WAR_LIST,
		GUILD_WAR_END_LIST,
		WAR_POINT,
		MONEY_CHANGE,
	};
}

namespace EterBase {
	enum class PacketError {
		BufferUnderflow
	};

	template <typename T>
	class PacketResult {
	public:
		PacketResult(T v) : value_(v), is_error_(false) {}
		PacketResult() : is_error_(true) {} // Mock unexpected
		
		bool has_value() const { return !is_error_; }
		operator bool() const { return has_value(); }
		T value() const { return value_; }

	private:
		T value_;
		bool is_error_;
	};
}

namespace std {
	template <typename E>
	EterBase::PacketResult<TPacketGCGuild> unexpected(E) { return EterBase::PacketResult<TPacketGCGuild>(); }
	
	template <typename E>
	EterBase::PacketResult<TPacketGCGuildInfo> unexpected(E) { return EterBase::PacketResult<TPacketGCGuildInfo>(); }
	
	template <typename E>
	EterBase::PacketResult<TPacketGCGuildSubMember> unexpected(E) { return EterBase::PacketResult<TPacketGCGuildSubMember>(); }
	
	template <typename E>
	EterBase::PacketResult<TPacketGCGuildWar> unexpected(E) { return EterBase::PacketResult<TPacketGCGuildWar>(); }
}

namespace Client::Network {
class GuildPacketCodec {
public:
	static EterBase::PacketResult<TPacketGCGuild> Decode(std::span<const uint8_t> buffer) {
		if (buffer.size() < sizeof(TPacketGCGuild)) {
			return EterBase::PacketResult<TPacketGCGuild>();
		}
		TPacketGCGuild pack;
		std::memcpy(&pack, buffer.data(), sizeof(TPacketGCGuild));
		return pack;
	}

	static EterBase::PacketResult<TPacketGCGuildInfo> DecodeInfo(std::span<const uint8_t> buffer) {
		if (buffer.size() < sizeof(TPacketGCGuildInfo)) {
			return EterBase::PacketResult<TPacketGCGuildInfo>();
		}
		TPacketGCGuildInfo pack;
		std::memcpy(&pack, buffer.data(), sizeof(TPacketGCGuildInfo));
		return pack;
	}

	static EterBase::PacketResult<TPacketGCGuildSubMember> DecodeMember(std::span<const uint8_t> buffer) {
		if (buffer.size() < sizeof(TPacketGCGuildSubMember)) {
			return EterBase::PacketResult<TPacketGCGuildSubMember>();
		}
		TPacketGCGuildSubMember pack;
		std::memcpy(&pack, buffer.data(), sizeof(TPacketGCGuildSubMember));
		return pack;
	}

	static EterBase::PacketResult<TPacketGCGuildWar> DecodeWar(std::span<const uint8_t> buffer) {
		if (buffer.size() < sizeof(TPacketGCGuildWar)) {
			return EterBase::PacketResult<TPacketGCGuildWar>();
		}
		TPacketGCGuildWar pack;
		std::memcpy(&pack, buffer.data(), sizeof(TPacketGCGuildWar));
		return pack;
	}
};
} // namespace Client::Network


// Mock CPythonNetworkStream
class CPythonNetworkStream {};

// Mock Singleton Base
template <typename T>
class CSingleton {
public:
	static T& Instance() { static T inst; return inst; }
};

// Mock CPythonPlayer
class CPythonPlayer : public CSingleton<CPythonPlayer> {
public:
	const char* GetName() { return "TestPlayer"; }
};

// Mock CPythonMessenger
class CPythonMessenger : public CSingleton<CPythonMessenger> {
public:
	std::string last_login;
	std::string last_logout;

	void LoginGuildMember(const char* name) { last_login = name; }
	void LogoutGuildMember(const char* name) { last_logout = name; }
};

#include "../src/UserInterface/PythonNetworkStreamPhaseGameGuild.h"

// Mock CPythonGuild
class CPythonGuild : public CSingleton<CPythonGuild> {
public:
	struct TGuildMemberData {
		std::string strName;
	};

	struct TGuildInfo {
		uint32_t dwGuildID;
		uint32_t dwMasterPID;
		uint32_t dwGuildLevel;
		uint32_t dwCurrentExperience;
		uint32_t dwCurrentMemberCount;
		uint32_t dwMaxMemberCount;
		uint32_t dwGuildMoney;
		uint8_t bHasLand;
		char szGuildName[GUILD_NAME_MAX_LEN + 1];
	};

	TGuildMemberData mock_member;
	TGuildInfo mock_info;
	bool guild_enabled = false;

	bool GetMemberDataPtrByPID(uint32_t dwPID, TGuildMemberData** ppData) {
		mock_member.strName = "MockMemberName";
		*ppData = &mock_member;
		return true;
	}

	void EnableGuild() { guild_enabled = true; }
	TGuildInfo& GetGuildInfoRef() { return mock_info; }
};


// Tests
TEST_CASE("PhaseGameGuildBridge - Login") {
	CPythonNetworkStream stream;
	TPacketGCGuild packet{};
	packet.header = 1;
	packet.size = sizeof(packet);
	packet.subheader = GuildSub::GC::LOGIN;

	CPythonMessenger::Instance().last_login = "";
	
	bool result = PhaseGameGuildBridge::HandleGuild(&stream, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
	
	CHECK(result == true);
	CHECK(CPythonMessenger::Instance().last_login == "MockMemberName");
}

TEST_CASE("PhaseGameGuildBridge - Logout") {
	CPythonNetworkStream stream;
	TPacketGCGuild packet{};
	packet.header = 1;
	packet.size = sizeof(packet);
	packet.subheader = GuildSub::GC::LOGOUT;

	CPythonMessenger::Instance().last_logout = "";
	
	bool result = PhaseGameGuildBridge::HandleGuild(&stream, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
	
	CHECK(result == true);
	CHECK(CPythonMessenger::Instance().last_logout == "MockMemberName");
}

TEST_CASE("PhaseGameGuildBridge - Info") {
	TPacketGCGuildInfo info{};
	info.guild_id = 42;
	info.master_pid = 1337;
	info.level = 10;
	info.exp = 5000;
	info.member_count = 25;
	info.max_member_count = 50;
	info.gold = 1000000;
	info.hasLand = 1;
	strncpy(info.name, "TestGuild", GUILD_NAME_MAX_LEN);

	CPythonGuild::Instance().guild_enabled = false;
	
	bool result = PhaseGameGuildBridge::HandleGuildSub_Info(info);
	
	CHECK(result == true);
	CHECK(CPythonGuild::Instance().guild_enabled == true);
	
	auto& stored_info = CPythonGuild::Instance().GetGuildInfoRef();
	CHECK(stored_info.dwGuildID == 42);
	CHECK(stored_info.dwMasterPID == 1337);
	CHECK(stored_info.dwGuildLevel == 10);
	CHECK(stored_info.dwCurrentExperience == 5000);
	CHECK(stored_info.dwCurrentMemberCount == 25);
	CHECK(stored_info.dwMaxMemberCount == 50);
	CHECK(stored_info.dwGuildMoney == 1000000);
	CHECK(stored_info.bHasLand == 1);
	CHECK(std::string(stored_info.szGuildName) == "TestGuild");
}
