#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include <expected>
#include <format>
#include <string_view>

namespace Client::IPC {

// ZASADA ZERO-CONFLICT: Brak starych makr, enum class z C++11, formatowanie C++20/23

enum class IpcOpcode : uint16_t {
    Ping = 0,
    AuthRequest,
    AuthResponse,
    // Dodatkowe opcody według potrzeb
    Unknown = 0xFFFF
};

enum class IpcCodecError : uint8_t {
    None = 0,
    InvalidMagic,
    UnsupportedVersion,
    BufferTooSmall,
    ChecksumMismatch
};

[[nodiscard]] constexpr std::string_view ToString(IpcCodecError err) noexcept {
    switch (err) {
        case IpcCodecError::None: return "None";
        case IpcCodecError::InvalidMagic: return "InvalidMagic";
        case IpcCodecError::UnsupportedVersion: return "UnsupportedVersion";
        case IpcCodecError::BufferTooSmall: return "BufferTooSmall";
        case IpcCodecError::ChecksumMismatch: return "ChecksumMismatch";
    }
    return "UnknownCodecError";
}

#pragma pack(push, 1)
struct IpcFrameHeader {
    uint32_t magic;         // 0x4D324950 "M2IP"
    uint16_t version;       // 1
    uint16_t opcode;        // IpcOpcode
    uint32_t sequenceId;    // Kolejnosc
    uint32_t payloadLength; // Rozmiar payloadu
    uint32_t payloadCrc32;  // CRC32 IEEE 802.3 dla payloadu
};
#pragma pack(pop)

struct DecodedIpcFrame {
    IpcFrameHeader header;
    std::vector<uint8_t> payload;
};

class IPCProtocolCodec {
public:
    static constexpr uint32_t MAGIC_M2IP = 0x4D324950;
    static constexpr uint16_t VERSION = 1;

    /**
     * @brief Koduje ramke IPC do bufora bajtow (std::vector).
     * Oblicza CRC32 (IEEE 802.3) payloadu w locie.
     */
    [[nodiscard]] static std::vector<uint8_t> EncodeFrame(
        IpcOpcode opcode, 
        uint32_t sequenceId, 
        std::span<const uint8_t> payload) noexcept;

    /**
     * @brief Dekoduje bufor bajtow na zdekodowana ramke IPC.
     * Weryfikuje naglowek, rozmiar bufora i sume CRC32.
     * Zwraca std::expected z błędem w przypadku niepowodzenia.
     */
    [[nodiscard]] static std::expected<DecodedIpcFrame, IpcCodecError> DecodeFrame(
        std::span<const uint8_t> buffer) noexcept;
};

} // namespace Client::IPC

// Specjalizacja std::formatter dla IpcCodecError do wygodnego logowania
template <>
struct std::formatter<Client::IPC::IpcCodecError> : std::formatter<std::string_view> {
    auto format(Client::IPC::IpcCodecError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::IPC::ToString(err), ctx);
    }
};
