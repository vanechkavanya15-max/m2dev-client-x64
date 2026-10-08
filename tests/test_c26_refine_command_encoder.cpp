#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

// Define Windows types mocked for tests to compile cleanly without StdAfx.h
using BYTE = unsigned char;
using WORD = unsigned short;
using DWORD = unsigned int;
#define WORD_MAX 0xffff

// Fake some types before UserInterface/Packet.h
typedef struct TPixelPosition { float x, y, z; } TPixelPosition;
typedef unsigned int UINT;
typedef struct RECT { long left; long top; long right; long bottom; } RECT;

#include "../src/UserInterface/Packet.h"
#include "../src/Client/Network/RefineCommandEncoder.h"
#include <cstring>
#include <array>

using namespace Client::Network;

TEST_CASE("RefineCommandEncoder - Standard Refine") {
    SUBCASE("Valid slot encodes correctly") {
        EterBase::ItemSlot slot(10);
        uint8_t refineType = 1; // e.g., Scroll

        auto result = RefineCommandEncoder::EncodeStandardRefine(slot, refineType);
        REQUIRE(result.has_value());

        const auto& buffer = result.value();
        REQUIRE(buffer.size() == sizeof(TPacketCGRefine));

        TPacketCGRefine packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketCGRefine));

        CHECK(packet.header == CG::REFINE);
        CHECK(packet.length == sizeof(TPacketCGRefine));
        CHECK(packet.pos == 10);
        CHECK(packet.type == 1);
    }

    SUBCASE("Slot > 255 returns MalformedPayload") {
        EterBase::ItemSlot slot(300);
        auto result = RefineCommandEncoder::EncodeStandardRefine(slot, 0);

        REQUIRE(!result.has_value());
        CHECK(result.error() == EterBase::PacketError::MalformedPayload);
    }
}

TEST_CASE("RefineCommandEncoder - Dragon Soul Refine") {
    SUBCASE("Valid items encode correctly") {
        std::vector<TItemPos> items = {
            TItemPos(INVENTORY, 5),
            TItemPos(INVENTORY, 6)
        };

        uint8_t subType = DragonSoulRefineWindow_UPGRADE;
        auto result = RefineCommandEncoder::EncodeDragonSoulRefine(subType, items);
        REQUIRE(result.has_value());

        const auto& buffer = result.value();
        REQUIRE(buffer.size() == sizeof(TPacketCGDragonSoulRefine));

        TPacketCGDragonSoulRefine packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketCGDragonSoulRefine));

        CHECK(packet.header == CG::DRAGON_SOUL_REFINE);
        CHECK(packet.length == sizeof(TPacketCGDragonSoulRefine));
        CHECK(packet.bSubType == subType);

        CHECK(packet.ItemGrid[0].window_type == INVENTORY);
        CHECK(packet.ItemGrid[0].cell == 5);
        CHECK(packet.ItemGrid[1].window_type == INVENTORY);
        CHECK(packet.ItemGrid[1].cell == 6);

        // Check padding
        for (size_t i = 2; i < DS_REFINE_WINDOW_MAX_NUM; ++i) {
            CHECK(packet.ItemGrid[i].window_type == INVENTORY);
            CHECK(packet.ItemGrid[i].cell == WORD_MAX);
        }
    }

    SUBCASE("Grid items exceeding DS_REFINE_WINDOW_MAX_NUM returns MalformedPayload") {
        std::vector<TItemPos> items(DS_REFINE_WINDOW_MAX_NUM + 1, TItemPos(INVENTORY, 0));

        auto result = RefineCommandEncoder::EncodeDragonSoulRefine(DragonSoulRefineWindow_REFINE, items);

        REQUIRE(!result.has_value());
        CHECK(result.error() == EterBase::PacketError::MalformedPayload);
    }
}
