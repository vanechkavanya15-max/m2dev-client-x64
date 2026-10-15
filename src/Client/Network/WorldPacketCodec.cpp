#include "WorldPacketCodec.h"

#ifndef _USERINTERFACE_PACKETS_PACKETHEADER_H_
// Note: Based on grep, we couldn't find UserInterface/Packets/PacketHeader.h, so assuming it's all in UserInterface/Packet.h
// or maybe there is a header CG::FISHING in UserInterface/Packet.h already.
#endif

#include <cstring>

namespace Client::Network {

    PacketResult<TPacketGCTime> WorldPacketCodec::DecodeTime(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCTime)) {
            return std::unexpected(PacketError::BufferUnderflow);
        }
        TPacketGCTime packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCTime));
        return packet;
    }

    PacketResult<TPacketGCDungeon> WorldPacketCodec::DecodeDungeon(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCDungeon)) {
            return std::unexpected(PacketError::BufferUnderflow);
        }
        TPacketGCDungeon packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCDungeon));
        return packet;
    }

    PacketResult<TPacketGCFishing> WorldPacketCodec::DecodeFishing(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCFishing)) {
            return std::unexpected(PacketError::BufferUnderflow);
        }
        TPacketGCFishing packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCFishing));
        return packet;
    }

    PacketResult<TPacketGCChannel> WorldPacketCodec::DecodeChannel(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCChannel)) {
            return std::unexpected(PacketError::BufferUnderflow);
        }
        TPacketGCChannel packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCChannel));
        return packet;
    }

    std::vector<uint8_t> WorldPacketCodec::EncodeFishing(int32_t rotation) {
        TPacketCGFishing packet{};
        packet.header = CG::FISHING;
        packet.length = sizeof(TPacketCGFishing);
        packet.dir = static_cast<uint8_t>(rotation / 5);

        std::vector<uint8_t> buffer(sizeof(TPacketCGFishing));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGFishing));
        return buffer;
    }

}
