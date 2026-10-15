#pragma once

#include <span>
#include <array>
#include <cstdint>
#include "../../EterBase/Result.h"
#include "ZeroCopyPacketBuffer.h"

namespace Client::Network {

/**
 * @class StreamPacketFramer
 * @brief Ekstraktor pelnych pakietow z bufora kolowego ZeroCopyPacketBuffer.
 * 
 * Zapewnia odczyt pakietow stalych i dynamicznych bez alokacji pamieci (Zero-Copy).
 * Uzywa O(1) tablicy (std::array) do konfiguracji pakietow.
 */
class StreamPacketFramer {
public:
    struct PacketConfig {
        bool isDynamic = false;
        size_t size = 0; // Rozmiar staly jesli isDynamic=false, maksymalny jesli isDynamic=true
        size_t lengthOffset = 0; // Offset pola dlugosci (dla dynamicznych)
        size_t lengthSize = 0; // Rozmiar pola dlugosci w bajtach (1, 2 lub 4) (dla dynamicznych)
    };

    constexpr StreamPacketFramer() noexcept : m_configs{} {}
    ~StreamPacketFramer() = default;

    // Brak kopiowania i przenoszenia (choc mozliwe, zwykle framer jest uzywany stacjonarnie)
    StreamPacketFramer(const StreamPacketFramer&) = delete;
    StreamPacketFramer& operator=(const StreamPacketFramer&) = delete;
    StreamPacketFramer(StreamPacketFramer&&) noexcept = default;
    StreamPacketFramer& operator=(StreamPacketFramer&&) noexcept = default;

    /**
     * @brief Rejestruje pakiet o stalym rozmiarze.
     */
    constexpr void RegisterFixedSize(uint8_t opcode, size_t size) noexcept {
        m_configs[opcode] = PacketConfig{false, size, 0, 0};
    }

    /**
     * @brief Rejestruje pakiet o dynamicznym rozmiarze.
     */
    constexpr void RegisterDynamicSize(uint8_t opcode, size_t lengthOffset, size_t lengthFieldSize, size_t maxSize) noexcept {
        m_configs[opcode] = PacketConfig{true, maxSize, lengthOffset, lengthFieldSize};
    }

    /**
     * @brief Wyodrebnia nastepny pelny pakiet z bufora sieciowego bez kopiowania.
     * @param buffer Bufor sieciowy (ZeroCopyPacketBuffer).
     * @return Zwraca widok (span) na wyodrebniony pakiet lub blad jesli pakiet jest niekompletny/bledny.
     */
    [[nodiscard]] EterBase::Result<std::span<const uint8_t>, EterBase::PacketError> FrameNextPacket(ZeroCopyPacketBuffer& buffer) noexcept {
        if (!buffer.HasBytes(1)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        auto opcodeSpan = buffer.PeekContiguous(1);
        if (opcodeSpan.empty()) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        uint8_t opcode = opcodeSpan[0];
        const auto& config = m_configs[opcode];

        // Jesli size == 0, pakiet nie jest zarejestrowany
        if (config.size == 0) {
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }

        size_t expectedSize = 0;

        if (!config.isDynamic) {
            expectedSize = config.size;
        } else {
            size_t minSizeForLength = config.lengthOffset + config.lengthSize;
            if (!buffer.HasBytes(minSizeForLength)) {
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            auto headerSpan = buffer.PeekContiguous(minSizeForLength);
            // Zabezpieczenie przed wrap-around dla naglowka
            if (headerSpan.size() < minSizeForLength) {
                // W skrajnym przypadku naglowek moze byc owiniety (wrap-around) w ZeroCopyPacketBuffer,
                // ale PeekContiguous obsluguje to automatycznie zwracajac ciagly widok przez scratch buffer.
                // Jesli nadal za krotki, to blad
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            size_t length = 0;
            if (config.lengthSize == 1) {
                length = headerSpan[config.lengthOffset];
            } else if (config.lengthSize == 2) {
                length = headerSpan[config.lengthOffset] | (static_cast<size_t>(headerSpan[config.lengthOffset + 1]) << 8);
            } else if (config.lengthSize == 4) {
                length = headerSpan[config.lengthOffset] | 
                         (static_cast<size_t>(headerSpan[config.lengthOffset + 1]) << 8) |
                         (static_cast<size_t>(headerSpan[config.lengthOffset + 2]) << 16) |
                         (static_cast<size_t>(headerSpan[config.lengthOffset + 3]) << 24);
            } else {
                return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
            }

            if (length < minSizeForLength || length > config.size) {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            expectedSize = length;
        }

        if (!buffer.HasBytes(expectedSize)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        auto payload = buffer.PeekContiguous(expectedSize);
        if (payload.size() < expectedSize) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        // Konsumujemy dane z bufora
        if (!buffer.CommitRead(expectedSize)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow); // Nie powinno sie zdarzyc
        }

        return payload;
    }

private:
    std::array<PacketConfig, 256> m_configs;
};

} // namespace Client::Network
