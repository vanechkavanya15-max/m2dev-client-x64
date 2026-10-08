#include <cstdint>
#include <expected>
#include <span>
#include <vector>

namespace EterBase {
    enum class PacketError { BufferUnderflow };
    template <typename T>
    using PacketResult = std::expected<T, PacketError>;
}

// These are basically defined in src/UserInterface/Packet.h
#define GUILD_GRADE_NAME_MAX_LEN 8
#define CHARACTER_ME_MAX_LEN 24 // From memory / standard Metin2
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

// ... we have to implement
// DecodeGuildHeader(std::span<const uint8_t> buffer) -> PacketResult<TPacketGCGuild>
// DecodeGuildSubInfo(std::span<const uint8_t> buffer) -> PacketResult<TPacketGCGuildSubInfo>
// ...
int main() {}
