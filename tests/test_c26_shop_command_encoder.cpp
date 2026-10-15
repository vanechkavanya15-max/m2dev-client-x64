#include <cassert>
#include <iostream>
#include <vector>
#include <cstring>
#include "../src/Client/Network/ShopCommandEncoder.h"

using namespace Client::Network;

void Test_EncodeBuy()
{
    ShopBuyCommand cmd;
    cmd.count = 5;
    cmd.position = EterBase::ItemSlot(10);

    auto result = ShopCommandEncoder::EncodeBuy(cmd);
    assert(result.has_value());

    const auto& buffer = result.value();
    assert(buffer.size() == sizeof(TPacketCGShop) + 2);

    TPacketCGShop decodedHeader;
    std::memcpy(&decodedHeader, buffer.data(), sizeof(TPacketCGShop));

    assert(decodedHeader.header == CG::SHOP);
    assert(decodedHeader.length == sizeof(TPacketCGShop) + 2);
    assert(decodedHeader.subheader == ShopSub::CG::BUY);

    uint8_t decodedCount;
    std::memcpy(&decodedCount, buffer.data() + sizeof(TPacketCGShop), 1);
    assert(decodedCount == 5);

    uint8_t decodedPosition;
    std::memcpy(&decodedPosition, buffer.data() + sizeof(TPacketCGShop) + 1, 1);
    assert(decodedPosition == 10);
}

void Test_EncodeSell()
{
    ShopSellCommand cmd;
    cmd.slot = EterBase::ItemSlot(15);
    cmd.count = 3;

    auto result = ShopCommandEncoder::EncodeSell(cmd);
    assert(result.has_value());

    const auto& buffer = result.value();
    assert(buffer.size() == sizeof(TPacketCGShop) + 2);

    TPacketCGShop decodedHeader;
    std::memcpy(&decodedHeader, buffer.data(), sizeof(TPacketCGShop));

    assert(decodedHeader.header == CG::SHOP);
    assert(decodedHeader.length == sizeof(TPacketCGShop) + 2);
    assert(decodedHeader.subheader == ShopSub::CG::SELL);

    uint8_t decodedSlot;
    std::memcpy(&decodedSlot, buffer.data() + sizeof(TPacketCGShop), 1);
    assert(decodedSlot == 15);

    uint8_t decodedCount;
    std::memcpy(&decodedCount, buffer.data() + sizeof(TPacketCGShop) + 1, 1);
    assert(decodedCount == 3);
}

int main()
{
    Test_EncodeBuy();
    Test_EncodeSell();

    std::cout << "All ShopCommandEncoder tests passed successfully!" << std::endl;
    return 0;
}
