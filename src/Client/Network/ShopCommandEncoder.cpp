#include "ShopCommandEncoder.h"
#include <cstring>

namespace Client::Network {

[[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> ShopCommandEncoder::EncodeBuy(const ShopBuyCommand& cmd) {
    TPacketCGShop headerPacket{};
    headerPacket.header = CG::SHOP;
    headerPacket.length = sizeof(TPacketCGShop) + sizeof(uint8_t) + sizeof(uint8_t);
    headerPacket.subheader = ShopSub::CG::BUY;

    std::vector<uint8_t> buffer(headerPacket.length);
    size_t offset = 0;

    std::memcpy(buffer.data() + offset, &headerPacket, sizeof(TPacketCGShop));
    offset += sizeof(TPacketCGShop);

    std::memcpy(buffer.data() + offset, &cmd.count, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    uint8_t rawPosition = static_cast<uint8_t>(cmd.position.value());
    std::memcpy(buffer.data() + offset, &rawPosition, sizeof(uint8_t));

    return buffer;
}

[[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> ShopCommandEncoder::EncodeSell(const ShopSellCommand& cmd) {
    TPacketCGShop headerPacket{};
    headerPacket.header = CG::SHOP;
    headerPacket.length = sizeof(TPacketCGShop) + sizeof(uint8_t) + sizeof(uint8_t);
    headerPacket.subheader = ShopSub::CG::SELL;

    std::vector<uint8_t> buffer(headerPacket.length);
    size_t offset = 0;

    std::memcpy(buffer.data() + offset, &headerPacket, sizeof(TPacketCGShop));
    offset += sizeof(TPacketCGShop);

    uint8_t rawSlot = static_cast<uint8_t>(cmd.slot.value());
    std::memcpy(buffer.data() + offset, &rawSlot, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    std::memcpy(buffer.data() + offset, &cmd.count, sizeof(uint8_t));

    return buffer;
}

} // namespace Client::Network
