#include "RefinePacketCodec.h"
#include <cstring>

namespace Client::Network
{
    PacketResult<TPacketGCRefineInformation> RefinePacketCodec::DecodeRefineInformation(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCRefineInformation))
            return std::unexpected(PacketError::BufferUnderflow);
        
        TPacketGCRefineInformation packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCRefineInformation));
        return packet;
    }

    PacketResult<TPacketGCRefineInformationNew> RefinePacketCodec::DecodeRefineInformationNew(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCRefineInformationNew))
            return std::unexpected(PacketError::BufferUnderflow);
        
        TPacketGCRefineInformationNew packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCRefineInformationNew));
        return packet;
    }

    PacketResult<TPacketGCLoverInfo> RefinePacketCodec::DecodeLoverInfo(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCLoverInfo))
            return std::unexpected(PacketError::BufferUnderflow);
        
        TPacketGCLoverInfo packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCLoverInfo));
        return packet;
    }

    PacketResult<TPacketGCMessenger> RefinePacketCodec::DecodeMessenger(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCMessenger))
            return std::unexpected(PacketError::BufferUnderflow);
        
        TPacketGCMessenger packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCMessenger));
        return packet;
    }

    std::vector<uint8_t> RefinePacketCodec::EncodeRefine(uint8_t pos, uint8_t type)
    {
        TPacketCGRefine packet{};
        packet.header = 0x050C; // HEADER_CG_REFINE
        packet.length = sizeof(TPacketCGRefine);
        packet.pos = pos;
        packet.type = type;

        std::vector<uint8_t> buffer(sizeof(TPacketCGRefine));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGRefine));
        return buffer;
    }
}
