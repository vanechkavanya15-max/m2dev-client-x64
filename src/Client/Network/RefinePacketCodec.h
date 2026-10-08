#pragma once

#include <vector>
#include <span>
#include <cstdint>
#include <expected>

#ifndef PACKET_MOCK_H
#include "../../UserInterface/Packet.h"
#endif

#include "EterBase/Result.h"

namespace Client::Network
{
    using EterBase::PacketError;
    using EterBase::PacketResult;

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
