#pragma once

#include <vector>
#include <span>
#include <cstdint>
#include <expected>

#ifndef PACKET_MOCK_H
#include "../../UserInterface/Packet.h"
#endif

namespace Client::Network
{
    enum class PacketError
    {
        BufferUnderflow,
        InvalidHeader,
    };

    template <typename T>
    using PacketResult = std::expected<T, PacketError>;

    class RefinePacketCodec
    {
    public:
        static PacketResult<TPacketGCRefineInformation> DecodeRefineInformation(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCRefineInformationNew> DecodeRefineInformationNew(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCLoverInfo> DecodeLoverInfo(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCMessenger> DecodeMessenger(std::span<const uint8_t> buffer);

        static std::vector<uint8_t> EncodeRefine(uint8_t pos, uint8_t type);
    };
}
