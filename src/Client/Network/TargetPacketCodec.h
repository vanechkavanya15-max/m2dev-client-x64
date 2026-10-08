#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <expected>

#include "UserInterface/Packet.h"

namespace EterBase {
    enum class PacketError {
        BufferUnderflow,
        InvalidHeader
    };

    template <typename T>
    using PacketResult = std::expected<T, PacketError>;
}

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
