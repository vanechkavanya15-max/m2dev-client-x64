#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <expected>

#include "EterBase/Result.h"
#include "Protocol/Protocol.h"

namespace Client::Network
{

class TargetPacketCodec
{
public:
    static EterBase::PacketResult<TPacketGCTarget> DecodeTarget(std::span<const uint8_t> buffer);
    static EterBase::PacketResult<TPacketGCTargetCreate> DecodeTargetCreate(std::span<const uint8_t> buffer);
    static EterBase::PacketResult<TPacketGCTargetUpdate> DecodeTargetUpdate(std::span<const uint8_t> buffer);
    static EterBase::PacketResult<TPacketGCTargetDelete> DecodeTargetDelete(std::span<const uint8_t> buffer);
    static EterBase::PacketResult<TPacketGCCreateFly> DecodeCreateFly(std::span<const uint8_t> buffer);

    static std::vector<uint8_t> EncodeFlyTargeting(uint32_t targetVid, float x, float y, float z);
};

} // namespace Client::Network
