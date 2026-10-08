#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#define PACKET_MOCK_H 1

#include <cstdint>
#include <vector>
#include <span>
#include <cstring>
#include <expected>

#pragma pack(push, 1)

constexpr uint16_t HEADER_CG_REFINE = 0x050C;
constexpr uint16_t HEADER_GC_REFINE_INFORMATION = 0x051D;

typedef struct command_refine
{
	uint16_t	header;
	uint16_t	length;
	uint8_t		pos;
	uint8_t		type;
} TPacketCGRefine;

#define REFINE_MATERIAL_MAX_NUM 5

struct TMaterial
{
    uint32_t vnum;
    int32_t  count;
};

typedef struct SRefineTable
{
    uint32_t src_vnum;
    uint32_t result_vnum;
    uint8_t material_count;
    int32_t cost;
    int32_t prob;
    TMaterial materials[REFINE_MATERIAL_MAX_NUM];
} TRefineTable;

typedef struct SPacketGCRefineInformation
{
    uint16_t header;
    uint16_t length;
    uint8_t  type;
    uint8_t  pos;
    TRefineTable refine_table;
} TPacketGCRefineInformation;

typedef struct SPacketGCRefineInformationNew
{
	uint16_t	header;
	uint16_t	length;
	uint8_t			type;
	uint8_t			pos;
	TRefineTable	refine_table;
} TPacketGCRefineInformationNew;

#define CHARACTER_NAME_MAX_LEN 24

typedef struct packet_lover_info
{
	uint16_t	header;
	uint16_t	length;
	char szName[CHARACTER_NAME_MAX_LEN + 1];
	uint8_t byLovePoint;
} TPacketGCLoverInfo;

typedef struct packet_messenger
{
    uint16_t	header;
    uint16_t	length;
    uint8_t subheader;
} TPacketGCMessenger;

#pragma pack(pop)

#include "../src/Client/Network/RefinePacketCodec.h"
#include "../src/Client/Network/RefinePacketCodec.cpp"

using namespace Client::Network;

TEST_CASE("RefinePacketCodec - DecodeRefineInformation Success") {
    TPacketGCRefineInformation packet{};
    packet.header = HEADER_GC_REFINE_INFORMATION;
    packet.length = sizeof(TPacketGCRefineInformation);
    packet.type = 1;
    packet.pos = 2;
    
    std::vector<uint8_t> buffer(sizeof(TPacketGCRefineInformation));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketGCRefineInformation));
    
    auto result = RefinePacketCodec::DecodeRefineInformation(buffer);
    REQUIRE(result.has_value());
    CHECK(result->type == 1);
    CHECK(result->pos == 2);
}

TEST_CASE("RefinePacketCodec - DecodeRefineInformation BufferUnderflow") {
    std::vector<uint8_t> buffer(sizeof(TPacketGCRefineInformation) - 1);
    auto result = RefinePacketCodec::DecodeRefineInformation(buffer);
    REQUIRE(!result.has_value());
    CHECK(result.error() == PacketError::BufferUnderflow);
}

TEST_CASE("RefinePacketCodec - EncodeRefine") {
    auto buffer = RefinePacketCodec::EncodeRefine(3, 4);
    REQUIRE(buffer.size() == sizeof(TPacketCGRefine));
    
    TPacketCGRefine packet;
    std::memcpy(&packet, buffer.data(), sizeof(TPacketCGRefine));
    
    CHECK(packet.pos == 3);
    CHECK(packet.type == 4);
    CHECK(packet.length == sizeof(TPacketCGRefine));
}

TEST_CASE("RefinePacketCodec - DecodeRefineInformationNew") {
    TPacketGCRefineInformationNew packet{};
    packet.type = 5;
    
    std::vector<uint8_t> buffer(sizeof(TPacketGCRefineInformationNew));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketGCRefineInformationNew));
    
    auto result = RefinePacketCodec::DecodeRefineInformationNew(buffer);
    REQUIRE(result.has_value());
    CHECK(result->type == 5);
}

TEST_CASE("RefinePacketCodec - DecodeLoverInfo") {
    TPacketGCLoverInfo packet{};
    packet.byLovePoint = 100;
    
    std::vector<uint8_t> buffer(sizeof(TPacketGCLoverInfo));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketGCLoverInfo));
    
    auto result = RefinePacketCodec::DecodeLoverInfo(buffer);
    REQUIRE(result.has_value());
    CHECK(result->byLovePoint == 100);
}

TEST_CASE("RefinePacketCodec - DecodeMessenger") {
    TPacketGCMessenger packet{};
    packet.subheader = 42;
    
    std::vector<uint8_t> buffer(sizeof(TPacketGCMessenger));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketGCMessenger));
    
    auto result = RefinePacketCodec::DecodeMessenger(buffer);
    REQUIRE(result.has_value());
    CHECK(result->subheader == 42);
}
