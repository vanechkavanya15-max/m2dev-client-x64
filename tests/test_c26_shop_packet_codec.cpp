#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "Client/Network/ShopPacketCodec.h"
#include "UserInterface/BeaviumProtocol.h"

using namespace Client::Network;

TEST_SUITE("ShopPacketCodec")
{
    TEST_CASE("DecodeShopStart")
    {
        SUBCASE("Success with exact size")
        {
            std::vector<uint8_t> buffer(sizeof(TPacketGCShopStart), 0xAA);
            auto result = ShopPacketCodec::DecodeShopStart(buffer);
            REQUIRE(result.has_value());
        }

        SUBCASE("Fails with truncated size")
        {
            std::vector<uint8_t> buffer(sizeof(TPacketGCShopStart) - 1, 0xAA);
            auto result = ShopPacketCodec::DecodeShopStart(buffer);
            REQUIRE_FALSE(result.has_value());
            CHECK(result.error() == PacketError::BufferUnderflow);
        }
    }

    TEST_CASE("DecodeShopStartEx")
    {
        SUBCASE("Success with exact size")
        {
            std::vector<uint8_t> buffer(sizeof(TPacketGCShopStartEx), 0xBB);
            auto result = ShopPacketCodec::DecodeShopStartEx(buffer);
            REQUIRE(result.has_value());
        }

        SUBCASE("Fails with truncated size")
        {
            std::vector<uint8_t> buffer(sizeof(TPacketGCShopStartEx) - 1, 0xBB);
            auto result = ShopPacketCodec::DecodeShopStartEx(buffer);
            REQUIRE_FALSE(result.has_value());
            CHECK(result.error() == PacketError::BufferUnderflow);
        }
    }

    TEST_CASE("DecodeShopUpdateItem")
    {
        SUBCASE("Success with exact size")
        {
            std::vector<uint8_t> buffer(sizeof(TPacketGCShopUpdateItem), 0xCC);
            auto result = ShopPacketCodec::DecodeShopUpdateItem(buffer);
            REQUIRE(result.has_value());
        }

        SUBCASE("Fails with truncated size")
        {
            std::vector<uint8_t> buffer(sizeof(TPacketGCShopUpdateItem) - 1, 0xCC);
            auto result = ShopPacketCodec::DecodeShopUpdateItem(buffer);
            REQUIRE_FALSE(result.has_value());
            CHECK(result.error() == PacketError::BufferUnderflow);
        }
    }

    TEST_CASE("DecodeShopSign")
    {
        SUBCASE("Success with exact size")
        {
            std::vector<uint8_t> buffer(sizeof(TPacketGCShopSign), 0xDD);
            auto result = ShopPacketCodec::DecodeShopSign(buffer);
            REQUIRE(result.has_value());
        }

        SUBCASE("Fails with truncated size")
        {
            std::vector<uint8_t> buffer(sizeof(TPacketGCShopSign) - 1, 0xDD);
            auto result = ShopPacketCodec::DecodeShopSign(buffer);
            REQUIRE_FALSE(result.has_value());
            CHECK(result.error() == PacketError::BufferUnderflow);
        }
    }

    TEST_CASE("EncodeShopBuy")
    {
        uint8_t pos = 42;
        auto buffer = ShopPacketCodec::EncodeShopBuy(pos);

        REQUIRE(buffer.size() == sizeof(Beavium::TPacketCGShopBuyBeavium));
        
        Beavium::TPacketCGShopBuyBeavium packet{};
        std::memcpy(&packet, buffer.data(), sizeof(packet));

        CHECK(packet.header == Beavium::CG::SHOP);
        CHECK(packet.subHeader == 0x01);
        CHECK(packet.count == 1);
        CHECK(packet.pos == 42);
        CHECK(packet.tab == 0);
    }

    TEST_CASE("EncodeShopSell")
    {
        uint8_t pos = 15;
        uint8_t count = 5; // count should be ignored based on implementation
        auto buffer = ShopPacketCodec::EncodeShopSell(pos, count);

        REQUIRE(buffer.size() == sizeof(Beavium::TPacketCGShopSellBeavium));

        Beavium::TPacketCGShopSellBeavium packet{};
        std::memcpy(&packet, buffer.data(), sizeof(packet));

        CHECK(packet.header == Beavium::CG::SHOP);
        CHECK(packet.subHeader == 0x02);
        CHECK(packet.windowType == 1);
        CHECK(packet.cell == 15);
    }
}
