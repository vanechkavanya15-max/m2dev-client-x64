#pragma once

#include <cstdint>
#include <expected>
#include <span>
#include <array>
#include "Client/Core/Result.h"

namespace Client::Network {

struct PacketHeaderInfo {
    bool isRegistered = false;
    uint32_t expectedSize = 0;
    bool isDynamic = false;
};

class PacketHeaderValidator {
public:
    constexpr PacketHeaderValidator() noexcept = default;

    /**
     * @brief Rejestruje naglowek pakietu do weryfikacji.
     * @param opcode Identyfikator pakietu.
     * @param size Oczekiwany rozmiar lub minimalny rozmiar w przypadku dynamicznego pakietu.
     * @param isDynamic Czy pakiet ma zmienny rozmiar.
     */
    constexpr void RegisterPacket(uint8_t opcode, uint32_t size, bool isDynamic = false) noexcept {
        m_headers[opcode] = {true, size, isDynamic};
    }

    /**
     * @brief Weryfikuje poczatek bufora (naglowek i rozmiar dostepnych danych).
     * @param buffer Bufor z danymi sieciowymi.
     * @return Zwraca opcode jesli pakiet jest poprawny, w przeciwnym razie PacketError.
     */
    [[nodiscard]] constexpr Core::Result<uint8_t, Core::PacketError> Validate(std::span<const uint8_t> buffer) const noexcept {
        if (buffer.empty()) {
            return std::unexpected(Core::PacketError::BufferUnderflow);
        }

        const uint8_t opcode = buffer[0];
        const auto& info = m_headers[opcode];

        if (!info.isRegistered) {
            return std::unexpected(Core::PacketError::InvalidHeader);
        }

        if (buffer.size() < info.expectedSize) {
            return std::unexpected(Core::PacketError::BufferUnderflow);
        }

        return opcode;
    }

    /**
     * @brief Sprawdza czy dany rozmiar pakietu jest prawidlowy.
     */
    [[nodiscard]] constexpr Core::Result<void, Core::PacketError> ValidateSize(uint8_t opcode, size_t actualSize) const noexcept {
        const auto& info = m_headers[opcode];
        if (!info.isRegistered) {
            return std::unexpected(Core::PacketError::InvalidHeader);
        }

        if (actualSize < info.expectedSize) {
            return std::unexpected(Core::PacketError::BufferUnderflow);
        }

        return {};
    }

    [[nodiscard]] constexpr bool IsRegistered(uint8_t opcode) const noexcept {
        return m_headers[opcode].isRegistered;
    }

    [[nodiscard]] constexpr bool IsDynamic(uint8_t opcode) const noexcept {
        return m_headers[opcode].isDynamic;
    }
    
    [[nodiscard]] constexpr uint32_t GetExpectedSize(uint8_t opcode) const noexcept {
        return m_headers[opcode].expectedSize;
    }

private:
    std::array<PacketHeaderInfo, 256> m_headers{};
};

} // namespace Client::Network
