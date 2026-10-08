#include "ShopPacketCodec.h"
#include "Protocol/BeaviumProtocol.h"
#include <cstring>

namespace Client::Network
{
    PacketResult<TPacketGCShopStart> ShopPacketCodec::DecodeShopStart(std::span<const uint8_t> buffer)
    {
        if (buffer.size_bytes() < sizeof(TPacketGCShopStart))
            return std::unexpected(PacketError::BufferUnderflow);

        TPacketGCShopStart packet{};
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCShopStart));
        return packet;
    }

    PacketResult<TPacketGCShopStartEx> ShopPacketCodec::DecodeShopStartEx(std::span<const uint8_t> buffer)
    {
        if (buffer.size_bytes() < sizeof(TPacketGCShopStartEx))
            return std::unexpected(PacketError::BufferUnderflow);

        TPacketGCShopStartEx packet{};
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCShopStartEx));
        return packet;
    }

    PacketResult<TPacketGCShopUpdateItem> ShopPacketCodec::DecodeShopUpdateItem(std::span<const uint8_t> buffer)
    {
        if (buffer.size_bytes() < sizeof(TPacketGCShopUpdateItem))
            return std::unexpected(PacketError::BufferUnderflow);

        TPacketGCShopUpdateItem packet{};
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCShopUpdateItem));
        return packet;
    }

    PacketResult<TPacketGCShopSign> ShopPacketCodec::DecodeShopSign(std::span<const uint8_t> buffer)
    {
        if (buffer.size_bytes() < sizeof(TPacketGCShopSign))
            return std::unexpected(PacketError::BufferUnderflow);

        TPacketGCShopSign packet{};
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCShopSign));
        return packet;
    }

    std::vector<uint8_t> ShopPacketCodec::EncodeShopBuy(uint8_t pos)
    {
        Beavium::TPacketCGShopBuyBeavium packet{};
        packet.header = Beavium::CG::SHOP;
        packet.subHeader = 0x01; // BUY
        packet.count = 1;
        packet.pos = pos;
        packet.tab = 0;

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));
        return buffer;
    }

    std::vector<uint8_t> ShopPacketCodec::EncodeShopSell(uint8_t pos, uint8_t count)
    {
        (void)count; // Ignored as per legacy SendShopSellPacket implementation

        Beavium::TPacketCGShopSellBeavium packet{};
        packet.header = Beavium::CG::SHOP;
        packet.subHeader = 0x02; // SELL
        packet.windowType = 1; // INVENTORY
        packet.cell = pos;

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));
        return buffer;
    }
}
