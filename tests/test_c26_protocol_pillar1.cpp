#include <cassert>
#include <iostream>
#include <cstdint>
#include <cstring>

#include "Client/Network/Protocol/ProtocolTypes.h"
#include "Client/Network/Protocol/Protocol.h"
#include "Client/Network/Protocol/BeaviumProtocol.h"
#include "UserInterface/BeaviumProtocol.h"

int main()
{
    std::cout << "[Pillar 1 Test] Rozpoczynam testy czystego protokolu sieciowego C++23..." << std::endl;

    // 1. Walidacja stalych binarnych ProtocolTypes.h
    static_assert(ITEM_SOCKET_SLOT_MAX_NUM == 3, "ITEM_SOCKET_SLOT_MAX_NUM musi wynosic 3");
    static_assert(ITEM_ATTRIBUTE_SLOT_MAX_NUM == 7, "ITEM_ATTRIBUTE_SLOT_MAX_NUM musi wynosic 7");
    static_assert(DS_REFINE_WINDOW_MAX_NUM == 15, "DS_REFINE_WINDOW_MAX_NUM musi wynosic 15");
    static_assert(PACKET_HEADER_SIZE == 4, "PACKET_HEADER_SIZE musi wynosic 4");

    // 2. Walidacja dokladnych rozmiarow struktur bazowych ProtocolTypes.h
    static_assert(sizeof(TItemPos) == 3, "TItemPos musi miec dokladnie 3 bajty");
    static_assert(sizeof(SItemPos) == 3, "SItemPos musi miec dokladnie 3 bajty");
    static_assert(sizeof(TQuickSlot) == 2, "TQuickSlot musi miec dokladnie 2 bajty");
    static_assert(sizeof(SQuickSlot) == 2, "SQuickSlot musi miec dokladnie 2 bajty");
    static_assert(sizeof(TPlayerItemAttribute) == 3, "TPlayerItemAttribute musi miec dokladnie 3 bajty");
    static_assert(sizeof(TPixelPosition) == 12, "TPixelPosition musi miec dokladnie 12 bajtow (3x float)");
    static_assert(sizeof(SPixelPosition) == 12, "SPixelPosition musi miec dokladnie 12 bajtow (3x float)");

    // 3. Walidacja struktur pakietowych Protocol.h (framing uint16_t header + uint16_t length)
    static_assert(sizeof(TPacketCGItemUse) == 7, "TPacketCGItemUse: header(2) + length(2) + TItemPos(3) = 7");
    static_assert(sizeof(TPacketCGAttack) == 11, "TPacketCGAttack = 11 bajtow");
    static_assert(sizeof(TPacketCGMove) == 19, "TPacketCGMove = 19 bajtow");
    static_assert(sizeof(TPacketGCPhase) == 5, "TPacketGCPhase = 5 bajtow");
    static_assert(sizeof(TPacketGCPing) == 8, "TPacketGCPing = 8 bajtow");
    static_assert(sizeof(TPacketCGPong) == 4, "TPacketCGPong = 4 bajty");
    static_assert(sizeof(TPacketGCCharacterDelete) == 8, "TPacketGCCharacterDelete = 8 bajtow");
    static_assert(sizeof(TPacketGCWarp) == 18, "TPacketGCWarp = 18 bajtow");

    // 4. Walidacja struktur BeaviumProtocol (ramki x64)
    static_assert(sizeof(Beavium::TPacketCGMoveBeavium) == 24, "TPacketCGMoveBeavium = 24 bajty");
    static_assert(sizeof(Beavium::TPacketCGAttackBeavium) == 8, "TPacketCGAttackBeavium = 8 bajtow");
    static_assert(sizeof(Beavium::TPacketCGAttackAltBeavium) == 9, "TPacketCGAttackAltBeavium = 9 bajtow");
    static_assert(sizeof(Beavium::TPacketCGItemPickupBeavium) == 5, "TPacketCGItemPickupBeavium = 5 bajtow");
    static_assert(sizeof(Beavium::TPacketCGScriptAnswerBeavium) == 2, "TPacketCGScriptAnswerBeavium = 2 bajty");
    static_assert(sizeof(Beavium::TPacketCGLogin2Beavium) == 56, "TPacketCGLogin2Beavium = 56 bajtow");

    // 5. Test funkcjonalny operacji na typach
    TItemPos itemPos(2, 45);
    assert(itemPos.window_type == 2);
    assert(itemPos.cell == 45);
    assert(itemPos.IsValidItemPosition());

    TPlayerItemAttribute attr(1, 1500);
    assert(attr.bType == 1);
    assert(attr.sValue == 1500);

    TQuickSlot quickSlot(1, 4);
    assert(quickSlot.Type == 1);
    assert(quickSlot.Position == 4);

    TPixelPosition pos(100.5f, 200.25f, 50.0f);
    assert(pos.x == 100.5f);
    assert(pos.y == 200.25f);
    assert(pos.z == 50.0f);

    // 6. Test subheaderow Protocol.h
    assert(GuildSub::CG::ADD_MEMBER == 0);
    assert(ShopSub::GC::START == 0);
    assert(ExchangeSub::CG::START == 0);

    // 7. Test helpera rotacji Beavium
    uint32_t rotEncoded = Beavium::EncodeRotationMicrodegrees(180.0f);
    assert(rotEncoded == 180000000);

    std::cout << "test_c26_protocol_pillar1: ALL TESTS PASSED (100%)" << std::endl;
    return 0;
}
