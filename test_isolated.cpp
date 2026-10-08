#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <cstdint>
#include <expected>
#include <span>
#include <vector>
#include <cstring>

namespace EterBase {
    enum class PacketError { BufferUnderflow };
    template <typename T>
    using PacketResult = std::expected<T, PacketError>;
}

#define GUILD_GRADE_NAME_MAX_LEN 8
#define GUILD_NAME_MAX_LEN 12

struct TPacketGCGuild {
    uint16_t header;
    uint16_t length;
    uint8_t subheader;
};

struct TPacketGCGuildSubInfo {
    uint16_t member_count;
    uint16_t max_member_count;
    uint32_t guild_id;
    uint32_t master_pid;
    uint32_t exp;
    uint8_t level;
    char name[GUILD_NAME_MAX_LEN+1];
    uint32_t gold;
    uint8_t hasLand;
};

struct TPacketGCGuildSubMember {
    uint32_t pid;
    uint8_t byGrade;
    uint8_t byIsGeneral;
    uint8_t byJob;
    uint8_t byLevel;
    uint32_t dwOffer;
    uint8_t byNameFlag;
};

struct TPacketGCGuildSubWar {
    uint32_t dwGuildSelf;
    uint32_t dwGuildOpp;
    uint8_t bType;
    uint8_t bWarState;
};

struct TPacketCGGuild {
    uint16_t header;
    uint16_t length;
    uint8_t bySubHeader;
};

namespace GuildSub::CG {
    enum : uint8_t { ADD_MEMBER = 0, REMOVE_MEMBER = 1 };
}
namespace CG {
    constexpr uint16_t GUILD = 0x0720;
}

namespace Client::Network {
    EterBase::PacketResult<TPacketGCGuild> DecodeGuildHeader(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCGuild)) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }
        TPacketGCGuild packet{};
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCGuild));
        return packet;
    }

    EterBase::PacketResult<TPacketGCGuildSubInfo> DecodeGuildSubInfo(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCGuildSubInfo)) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }
        TPacketGCGuildSubInfo packet{};
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCGuildSubInfo));
        return packet;
    }

    EterBase::PacketResult<TPacketGCGuildSubMember> DecodeGuildSubMember(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCGuildSubMember)) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }
        TPacketGCGuildSubMember packet{};
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCGuildSubMember));
        return packet;
    }

    EterBase::PacketResult<TPacketGCGuildSubWar> DecodeGuildSubWar(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCGuildSubWar)) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }
        TPacketGCGuildSubWar packet{};
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCGuildSubWar));
        return packet;
    }

    std::vector<uint8_t> EncodeGuildAddMember(uint32_t vid) {
        std::vector<uint8_t> buffer(sizeof(TPacketCGGuild) + sizeof(uint32_t));
        TPacketCGGuild packet{};
        packet.header = CG::GUILD;
        packet.length = static_cast<uint16_t>(buffer.size());
        packet.bySubHeader = GuildSub::CG::ADD_MEMBER;
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGGuild));
        std::memcpy(buffer.data() + sizeof(TPacketCGGuild), &vid, sizeof(uint32_t));
        return buffer;
    }

    std::vector<uint8_t> EncodeGuildRemoveMember(uint32_t pid) {
        std::vector<uint8_t> buffer(sizeof(TPacketCGGuild) + sizeof(uint32_t));
        TPacketCGGuild packet{};
        packet.header = CG::GUILD;
        packet.length = static_cast<uint16_t>(buffer.size());
        packet.bySubHeader = GuildSub::CG::REMOVE_MEMBER;
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGGuild));
        std::memcpy(buffer.data() + sizeof(TPacketCGGuild), &pid, sizeof(uint32_t));
        return buffer;
    }
}

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
        std::vector<uint8_t> buffer(sizeof(mock));
        std::memcpy(buffer.data(), &mock, sizeof(mock));
        auto res = DecodeGuildSubInfo(buffer);
        REQUIRE(res.has_value());
        CHECK(res->guild_id == mock.guild_id);
    }
}
