#pragma once

#ifndef _USERINTERFACE_PACKET_H_
#include "Protocol/Protocol.h"
#endif

#include <cstdint>
#include <span>
#include <vector>
#include <expected>

#include "EterBase/Result.h"

namespace Client::Network {

    using EterBase::PacketError;
    using EterBase::PacketResult;

    class WorldPacketCodec {
    public:
        static PacketResult<TPacketGCTime> DecodeTime(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCDungeon> DecodeDungeon(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCFishing> DecodeFishing(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCChannel> DecodeChannel(std::span<const uint8_t> buffer);
        static std::vector<uint8_t> EncodeFishing(int32_t rotation);
    };

}
