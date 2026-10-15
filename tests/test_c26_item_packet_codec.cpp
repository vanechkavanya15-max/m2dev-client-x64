#include <iostream>
#include <cassert>
#include <vector>

// Mocks
#define D3D9_H
#define D3DX9_H

#include "../src/Client/Network/ItemPacketCodec.h"

using namespace Client::Network;

void TestItemUse() {
    TPacketCGItemUse original{};
    original.header = 1;
    original.pos = TItemPos(1, 10);

    auto encoded = ItemPacketCodec::EncodeItemUse(original);
    assert(encoded.has_value());

    auto decoded = ItemPacketCodec::DecodeItemUse(encoded.value());
    assert(decoded.has_value());
    assert(decoded->header == 1);
    assert(decoded->pos.window_type == 1);
    assert(decoded->pos.cell == 10);
    std::cout << "TestItemUse passed\n";
}

void TestItemMove() {
    TPacketCGItemMove original{};
    original.header = 2;
    original.pos = TItemPos(1, 5);
    original.change_pos = TItemPos(2, 10);
    original.num = 3;

    auto encoded = ItemPacketCodec::EncodeItemMove(original);
    assert(encoded.has_value());

    auto decoded = ItemPacketCodec::DecodeItemMove(encoded.value());
    assert(decoded.has_value());
    assert(decoded->header == 2);
    assert(decoded->pos.window_type == 1);
    assert(decoded->pos.cell == 5);
    assert(decoded->change_pos.window_type == 2);
    assert(decoded->change_pos.cell == 10);
    assert(decoded->num == 3);
    std::cout << "TestItemMove passed\n";
}

void TestItemDrop() {
    TPacketCGItemDrop original{};
    original.header = 3;
    original.pos = TItemPos(1, 20);
    original.elk = 5000;

    auto encoded = ItemPacketCodec::EncodeItemDrop(original);
    assert(encoded.has_value());

    auto decoded = ItemPacketCodec::DecodeItemDrop(encoded.value());
    assert(decoded.has_value());
    assert(decoded->header == 3);
    assert(decoded->pos.window_type == 1);
    assert(decoded->pos.cell == 20);
    assert(decoded->elk == 5000);
    std::cout << "TestItemDrop passed\n";
}

void TestItemSet() {
    TPacketGCItemSet original{};
    original.header = 4;
    original.pos = TItemPos(3, 15);
    original.vnum = 12345;
    original.count = 99;

    auto encoded = ItemPacketCodec::EncodeItemSet(original);
    assert(encoded.has_value());

    auto decoded = ItemPacketCodec::DecodeItemSet(encoded.value());
    assert(decoded.has_value());
    assert(decoded->header == 4);
    assert(decoded->pos.window_type == 3);
    assert(decoded->pos.cell == 15);
    assert(decoded->vnum == 12345);
    assert(decoded->count == 99);
    std::cout << "TestItemSet passed\n";
}

void TestItemDel() {
    TPacketGCItemDel original{};
    original.header = 5;
    original.pos = 42;

    auto encoded = ItemPacketCodec::EncodeItemDel(original);
    assert(encoded.has_value());

    auto decoded = ItemPacketCodec::DecodeItemDel(encoded.value());
    assert(decoded.has_value());
    assert(decoded->header == 5);
    assert(decoded->pos == 42);
    std::cout << "TestItemDel passed\n";
}

void TestItemGroundAdd() {
    TPacketGCItemGroundAdd original{};
    original.header = 6;
    original.pos = TItemPos(4, 50);
    original.vnum = 54321;
    original.count = 10;
    original.flags = 1;
    original.anti_flags = 2;

    auto encoded = ItemPacketCodec::EncodeItemGroundAdd(original);
    assert(encoded.has_value());

    auto decoded = ItemPacketCodec::DecodeItemGroundAdd(encoded.value());
    assert(decoded.has_value());
    assert(decoded->header == 6);
    assert(decoded->pos.window_type == 4);
    assert(decoded->pos.cell == 50);
    assert(decoded->vnum == 54321);
    assert(decoded->count == 10);
    assert(decoded->flags == 1);
    assert(decoded->anti_flags == 2);
    std::cout << "TestItemGroundAdd passed\n";
}

int main() {
    TestItemUse();
    TestItemMove();
    TestItemDrop();
    TestItemSet();
    TestItemDel();
    TestItemGroundAdd();
    std::cout << "All tests passed successfully.\n";
    return 0;
}

extern "C" void OutputDebugStringA(const char* msg) {
    std::cout << "[OutputDebugStringA] " << msg << "\n";
}
