#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/Client/Network/PacketSchemaEngine.h"
#include <vector>

using namespace Client::Network;

TEST_CASE("PacketSchemaEngine registers and reads U32 correctly") {
    PacketSchemaEngine engine;

    std::vector<PacketSchemaEngine::FieldOffset> schema = {
        {"header", 0, 1},
        {"size", 1, 2},
        {"id", 3, 4}
    };

    engine.RegisterPacketSchema(100, schema);

    std::vector<uint8_t> payload = {
        0x50,             // header
        0x10, 0x00,       // size
        0x44, 0x33, 0x22, 0x11 // id
    };

    SUBCASE("Read 1-byte field") {
        auto result = engine.GetFieldValueU32(100, "header", payload);
        REQUIRE(result.has_value());
        CHECK(result.value() == 0x50);
    }

    SUBCASE("Read 2-byte field") {
        auto result = engine.GetFieldValueU32(100, "size", payload);
        REQUIRE(result.has_value());
        CHECK(result.value() == 0x10);
    }

    SUBCASE("Read 4-byte field") {
        auto result = engine.GetFieldValueU32(100, "id", payload);
        REQUIRE(result.has_value());
        CHECK(result.value() == 0x11223344);
    }

    SUBCASE("Read non-existent field") {
        auto result = engine.GetFieldValueU32(100, "unknown", payload);
        REQUIRE(!result.has_value());
        CHECK(result.error() == "Field not found in schema");
    }

    SUBCASE("Buffer underflow") {
        std::vector<uint8_t> short_payload = { 0x50, 0x10 };
        auto result = engine.GetFieldValueU32(100, "id", short_payload);
        REQUIRE(!result.has_value());
        CHECK(result.error() == "Buffer underflow");
    }

    SUBCASE("Unknown opcode") {
        auto result = engine.GetFieldValueU32(999, "id", payload);
        REQUIRE(!result.has_value());
        CHECK(result.error() == "Schema not found for opcode");
    }
}
