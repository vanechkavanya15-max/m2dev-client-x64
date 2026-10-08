#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include "../src/Client/Network/ProtocolAdapter.h"
#include "../src/UserInterface/Packet.h"

using namespace Client::Network;

TEST_SUITE("ProtocolAdapter") {

    TEST_CASE("Classic Ymir Profile - 1:1 Mapping") {
        ProtocolAdapter adapter(ServerProfile::ClassicYmir);

        SUBCASE("Client to Server") {
            auto res = adapter.TranslateClientToServer(CG::ATTACK);
            REQUIRE(res.has_value());
            CHECK_EQ(res.value(), CG::ATTACK);
        }

        SUBCASE("Server to Client") {
            auto res = adapter.TranslateServerToClient(GC::MAIN_CHARACTER);
            REQUIRE(res.has_value());
            CHECK_EQ(res.value(), GC::MAIN_CHARACTER);
        }
    }

    TEST_CASE("Pandora Profile - Custom Mapping") {
        ProtocolAdapter adapter(ServerProfile::Pandora);

        SUBCASE("Client to Server - Valid mapped") {
            CHECK_EQ(adapter.TranslateClientToServer(CG::ATTACK).value(), 0x02);
            CHECK_EQ(adapter.TranslateClientToServer(CG::USE_SKILL).value(), 0x36);
            CHECK_EQ(adapter.TranslateClientToServer(CG::ITEM_PICKUP).value(), 0x0F);
            CHECK_EQ(adapter.TranslateClientToServer(CG::ON_CLICK).value(), 0x1A);
            CHECK_EQ(adapter.TranslateClientToServer(CG::SHOP).value(), 0x32);
            CHECK_EQ(adapter.TranslateClientToServer(CG::SCRIPT_ANSWER).value(), 0x1D);
            CHECK_EQ(adapter.TranslateClientToServer(CG::LOGIN2).value(), 0x01);
            CHECK_EQ(adapter.TranslateClientToServer(CG::MOVE).value(), 0x07);
        }
        
        SUBCASE("Client to Server - Unmapped defaults to same") {
            auto res = adapter.TranslateClientToServer(CG::WHISPER);
            REQUIRE(res.has_value());
            CHECK_EQ(res.value(), CG::WHISPER);
        }
        
        SUBCASE("Server to Client - Valid mapped") {
            CHECK_EQ(adapter.TranslateServerToClient(0x02).value(), CG::ATTACK);
            CHECK_EQ(adapter.TranslateServerToClient(0x36).value(), CG::USE_SKILL);
            CHECK_EQ(adapter.TranslateServerToClient(0x0F).value(), CG::ITEM_PICKUP);
            CHECK_EQ(adapter.TranslateServerToClient(0x1A).value(), CG::ON_CLICK);
            CHECK_EQ(adapter.TranslateServerToClient(0x32).value(), CG::SHOP);
            CHECK_EQ(adapter.TranslateServerToClient(0x1D).value(), CG::SCRIPT_ANSWER);
            CHECK_EQ(adapter.TranslateServerToClient(0x01).value(), CG::LOGIN2);
            CHECK_EQ(adapter.TranslateServerToClient(0x07).value(), CG::MOVE);
        }

        SUBCASE("Server to Client - Unmapped defaults to same") {
            auto res = adapter.TranslateServerToClient(GC::MAIN_CHARACTER);
            REQUIRE(res.has_value());
            CHECK_EQ(res.value(), GC::MAIN_CHARACTER);
        }
    }
}
