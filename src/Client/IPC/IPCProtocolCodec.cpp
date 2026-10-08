#include "IPCProtocolCodec.h"
#include "EterBase/CRC32.h"
#include <cstring>

namespace Client::IPC {

std::vector<uint8_t> IPCProtocolCodec::EncodeFrame(
    IpcOpcode opcode, 
    uint32_t sequenceId, 
    std::span<const uint8_t> payload) noexcept
{
    IpcFrameHeader header{};
    header.magic = MAGIC_M2IP;
    header.version = VERSION;
    header.opcode = static_cast<uint16_t>(opcode);
    header.sequenceId = sequenceId;
    header.payloadLength = static_cast<uint32_t>(payload.size());
    
    if (payload.empty()) {
        header.payloadCrc32 = 0; // Lub GetCRC32("", 0) zaleznie od konwencji, 0 jest bezpieczne dla pustego payloadu jesli tak zalozymy
        // EterBase GetCRC32 zwraca ^0xffffffff dla pustego, czyli 0xffffffff, ale uzyjmy wprost funckcji
        header.payloadCrc32 = GetCRC32(nullptr, 0); 
    } else {
        header.payloadCrc32 = GetCRC32(reinterpret_cast<const char*>(payload.data()), payload.size());
    }

    std::vector<uint8_t> frameBuffer;
    frameBuffer.resize(sizeof(IpcFrameHeader) + payload.size());

    std::memcpy(frameBuffer.data(), &header, sizeof(IpcFrameHeader));
    if (!payload.empty()) {
        std::memcpy(frameBuffer.data() + sizeof(IpcFrameHeader), payload.data(), payload.size());
    }

    return frameBuffer;
}

std::expected<DecodedIpcFrame, IpcCodecError> IPCProtocolCodec::DecodeFrame(
    std::span<const uint8_t> buffer) noexcept
{
    if (buffer.size() < sizeof(IpcFrameHeader)) {
        return std::unexpected(IpcCodecError::BufferTooSmall);
    }

    IpcFrameHeader header;
    std::memcpy(&header, buffer.data(), sizeof(IpcFrameHeader));

    if (header.magic != MAGIC_M2IP) {
        return std::unexpected(IpcCodecError::InvalidMagic);
    }

    if (header.version != VERSION) {
        return std::unexpected(IpcCodecError::UnsupportedVersion);
    }

    if (buffer.size() < sizeof(IpcFrameHeader) + header.payloadLength) {
        return std::unexpected(IpcCodecError::BufferTooSmall);
    }

    DecodedIpcFrame decodedFrame;
    decodedFrame.header = header;

    if (header.payloadLength > 0) {
        decodedFrame.payload.assign(
            buffer.data() + sizeof(IpcFrameHeader),
            buffer.data() + sizeof(IpcFrameHeader) + header.payloadLength
        );
        
        uint32_t computedCrc = GetCRC32(reinterpret_cast<const char*>(decodedFrame.payload.data()), decodedFrame.payload.size());
        if (computedCrc != header.payloadCrc32) {
            return std::unexpected(IpcCodecError::ChecksumMismatch);
        }
    } else {
        uint32_t computedCrc = GetCRC32(nullptr, 0);
        if (computedCrc != header.payloadCrc32) {
            return std::unexpected(IpcCodecError::ChecksumMismatch);
        }
    }

    return decodedFrame;
}

} // namespace Client::IPC
