#pragma once

#ifndef _USERINTERFACE_PACKET_H_
#include "../../UserInterface/Packet.h"
#endif

#include <cstdint>
#include <span>
#include <vector>
#include <expected>

namespace Client::Network {

    enum class PacketError {
        BufferTooSmall,
        InvalidHeader
    };

    template <typename T>
    using PacketResult = std::expected<T, PacketError>;

    class WorldPacketCodec {
    public:
        static PacketResult<TPacketGCTime> DecodeTime(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCDungeon> DecodeDungeon(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCFishing> DecodeFishing(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCChannel> DecodeChannel(std::span<const uint8_t> buffer);
        static std::vector<uint8_t> EncodeFishing(int32_t rotation);
    };

}
