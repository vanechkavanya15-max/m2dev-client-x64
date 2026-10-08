#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <expected>

#include "UserInterface/Packet.h"

namespace Client::Network
{
    enum class PacketError
    {
        BufferUnderflow
    };

    template<typename T>
    using PacketResult = std::expected<T, PacketError>;

    class ShopPacketCodec
    {
    public:
        static PacketResult<TPacketGCShopStart> DecodeShopStart(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCShopStartEx> DecodeShopStartEx(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCShopUpdateItem> DecodeShopUpdateItem(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCShopSign> DecodeShopSign(std::span<const uint8_t> buffer);

        static std::vector<uint8_t> EncodeShopBuy(uint8_t pos);
        static std::vector<uint8_t> EncodeShopSell(uint8_t pos, uint8_t count);
    };
}
