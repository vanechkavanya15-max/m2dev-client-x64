#include "TargetPacketCodec.h"
#include <cstring>

namespace Client::Network
{

EterBase::PacketResult<TPacketGCTarget> TargetPacketCodec::DecodeTarget(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(TPacketGCTarget))
    {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCTarget packet;
    std::memcpy(&packet, buffer.data(), sizeof(packet));

    if (packet.header != GC::TARGET)
    {
        return std::unexpected(EterBase::PacketError::InvalidHeader);
    }

    return packet;
}

EterBase::PacketResult<TPacketGCTargetCreate> TargetPacketCodec::DecodeTargetCreate(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(TPacketGCTargetCreate))
    {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCTargetCreate packet;
    std::memcpy(&packet, buffer.data(), sizeof(packet));
    packet.szTargetName[sizeof(packet.szTargetName) - 1] = '\0';

    return packet;
}

EterBase::PacketResult<TPacketGCTargetUpdate> TargetPacketCodec::DecodeTargetUpdate(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(TPacketGCTargetUpdate))
    {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCTargetUpdate packet;
    std::memcpy(&packet, buffer.data(), sizeof(packet));

    if (packet.header != GC::TARGET_UPDATE)
    {
        return std::unexpected(EterBase::PacketError::InvalidHeader);
    }

    return packet;
}

EterBase::PacketResult<TPacketGCTargetDelete> TargetPacketCodec::DecodeTargetDelete(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(TPacketGCTargetDelete))
    {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCTargetDelete packet;
    std::memcpy(&packet, buffer.data(), sizeof(packet));

    if (packet.header != GC::TARGET_DELETE)
    {
        return std::unexpected(EterBase::PacketError::InvalidHeader);
    }

    return packet;
}

EterBase::PacketResult<TPacketGCCreateFly> TargetPacketCodec::DecodeCreateFly(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(TPacketGCCreateFly))
    {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCCreateFly packet;
    std::memcpy(&packet, buffer.data(), sizeof(packet));

    if (packet.header != GC::CREATE_FLY)
    {
        return std::unexpected(EterBase::PacketError::InvalidHeader);
    }

    return packet;
}

std::vector<uint8_t> TargetPacketCodec::EncodeFlyTargeting(uint32_t targetVid, float x, float y, float z)
{
    TPacketCGFlyTargeting packet;
    packet.header = CG::FLY_TARGETING;
    packet.length = sizeof(packet);
    packet.dwTargetVID = targetVid;
    packet.lX = static_cast<int32_t>(x);
    packet.lY = static_cast<int32_t>(y);

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

} // namespace Client::Network
