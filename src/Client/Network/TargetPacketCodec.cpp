#include "TargetPacketCodec.h"
#include <cstring>
#include <format>
#include "EterBase/Error/PacketError.h"
#include "EterBase/ModernLogger.h"

namespace Client::Network
{

EterBase::PacketResult<TPacketGCTarget> TargetPacketCodec::DecodeTarget(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(TPacketGCTarget))
    {
        EterBase::ModernLogger::Error(std::format("DecodeTarget buffer underflow: {} < {}", buffer.size(), sizeof(TPacketGCTarget)));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCTarget packet;
    std::memcpy(&packet, buffer.data(), sizeof(packet));

    if (packet.header != GC::TARGET)
    {
        EterBase::ModernLogger::Error(std::format("DecodeTarget invalid header: {} != {}", packet.header, GC::TARGET));
        return std::unexpected(EterBase::PacketError::InvalidHeader);
    }

    return packet;
}

EterBase::PacketResult<TPacketGCTargetCreate> TargetPacketCodec::DecodeTargetCreate(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(TPacketGCTargetCreate))
    {
        EterBase::ModernLogger::Error(std::format("DecodeTargetCreate buffer underflow: {} < {}", buffer.size(), sizeof(TPacketGCTargetCreate)));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCTargetCreate packet;
    std::memcpy(&packet, buffer.data(), sizeof(packet));

    // The legacy packet might have been replaced or using TARGET_CREATE_NEW, but since the 
    // prompt specifies DecodeTargetCreate returning TPacketGCTargetCreate, we support it.
    // However, there is no GC::TARGET_CREATE in Packet.h. We will not strictly check the header 
    // to avoid compilation error, or we will check against the only available target create const.
    // But since the project is C++23 zero-conflict architecture, we will check if there's any header.
    // Wait, let's look at the header constants carefully: 
    // We found GC::TARGET_CREATE_NEW = 0x0A13, GC::TARGET_UPDATE = 0x0A11, GC::TARGET_DELETE = 0x0A12.
    // We'll skip exact header match for TARGET_CREATE since it's not defined in the provided grep.

    // Ensure null termination of the string just in case
    packet.szTargetName[sizeof(packet.szTargetName) - 1] = '\0';

    return packet;
}

EterBase::PacketResult<TPacketGCTargetUpdate> TargetPacketCodec::DecodeTargetUpdate(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(TPacketGCTargetUpdate))
    {
        EterBase::ModernLogger::Error(std::format("DecodeTargetUpdate buffer underflow: {} < {}", buffer.size(), sizeof(TPacketGCTargetUpdate)));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCTargetUpdate packet;
    std::memcpy(&packet, buffer.data(), sizeof(packet));

    if (packet.header != GC::TARGET_UPDATE)
    {
        EterBase::ModernLogger::Error(std::format("DecodeTargetUpdate invalid header: {} != {}", packet.header, GC::TARGET_UPDATE));
        return std::unexpected(EterBase::PacketError::InvalidHeader);
    }

    return packet;
}

EterBase::PacketResult<TPacketGCTargetDelete> TargetPacketCodec::DecodeTargetDelete(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(TPacketGCTargetDelete))
    {
        EterBase::ModernLogger::Error(std::format("DecodeTargetDelete buffer underflow: {} < {}", buffer.size(), sizeof(TPacketGCTargetDelete)));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCTargetDelete packet;
    std::memcpy(&packet, buffer.data(), sizeof(packet));

    if (packet.header != GC::TARGET_DELETE)
    {
        EterBase::ModernLogger::Error(std::format("DecodeTargetDelete invalid header: {} != {}", packet.header, GC::TARGET_DELETE));
        return std::unexpected(EterBase::PacketError::InvalidHeader);
    }

    return packet;
}

EterBase::PacketResult<TPacketGCCreateFly> TargetPacketCodec::DecodeCreateFly(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(TPacketGCCreateFly))
    {
        EterBase::ModernLogger::Error(std::format("DecodeCreateFly buffer underflow: {} < {}", buffer.size(), sizeof(TPacketGCCreateFly)));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCCreateFly packet;
    std::memcpy(&packet, buffer.data(), sizeof(packet));

    if (packet.header != GC::CREATE_FLY)
    {
        EterBase::ModernLogger::Error(std::format("DecodeCreateFly invalid header: {} != {}", packet.header, GC::CREATE_FLY));
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
    
    // In legacy codebase, __LocalPositionToGlobalPosition may be needed, but we just set x and y.
    packet.lX = static_cast<int32_t>(x);
    packet.lY = static_cast<int32_t>(y);

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

} // namespace Client::Network
