#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <optional>
#include <cstring>

#if defined(_M_X64) || defined(__x86_64__)
#include <nmmintrin.h>
#endif

#include "Result.h"
#include "LogModern.h"
#include "StrongTypes.h"
#include "UserInterface/Core/EventBus.h"

/**
 * @file SIMD_CRC32Hardware.h
 * @brief Sprzetowy, przyspieszony kalkulator CRC32 wykorzystujacy instrukcje x64 SSE4.2.
 * 
 * Zastepuje starsze implementacje CRC32 wymagajace duzych tablic. Wykorzystuje C++23.
 */

namespace EterBase {

/**
 * @brief Zdarzenie emitowane po udanym obliczeniu sumy kontrolnej.
 * 
 * Uzywane do odpiecia logiki EterBase od GUI i powiadomienia subskrybentow o
 * zweryfikowanych blokach danych.
 */
struct HardwareCrc32CalculatedEvent : public UserInterface::Core::IEvent {
    uint32_t checksum;
    size_t size;

    /**
     * @brief Konstruktor zdarzenia CRC32.
     * @param checksum Obliczona suma kontrolna.
     * @param size Rozmiar przetworzonych danych.
     */
    HardwareCrc32CalculatedEvent(uint32_t checksum, size_t size) 
        : checksum(checksum), size(size) {}
};

/**
 * @brief Klasa implementujaca sprzetowe obliczanie CRC32 (SSE4.2).
 */
class SIMD_CRC32Hardware {
public:
    /**
     * @brief Oblicza sprzetowo sume kontrolna CRC32-C dla zadanego bufora.
     * @param data Widok danych do przetworzenia.
     * @param initialCrc Poczatkowa wartosc sumy kontrolnej.
     * @return PacketResult<uint32_t> zawierajacy obliczona sume lub blad braku danych.
     */
    [[nodiscard]] static PacketResult<uint32_t> Calculate(std::span<const uint8_t> data, uint32_t initialCrc = 0xFFFFFFFF) noexcept {
        return ValidateSpan(data)
            .transform([initialCrc](std::span<const uint8_t> validData) {
                return ComputeInternal(validData, initialCrc);
            });
    }

    /**
     * @brief Oblicza sprzetowo sume kontrolna CRC32-C dla zadanego ciagu znakow.
     * @param text Ciag znakow do przetworzenia.
     * @return PacketResult<uint32_t> zawierajacy obliczona sume lub blad braku danych.
     */
    [[nodiscard]] static PacketResult<uint32_t> CalculateString(std::string_view text) noexcept {
        auto dataOpt = std::make_optional(text)
            .and_then([](std::string_view t) -> std::optional<std::span<const uint8_t>> {
                if (t.empty()) {
                    return std::nullopt;
                }
                return std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(t.data()), t.size());
            });

        if (dataOpt.has_value()) {
            return Calculate(dataOpt.value());
        }

        ModernLogger::Error("SIMD_CRC32Hardware::CalculateString: Buffer is empty.");
        return MakeError(PacketError::BufferUnderflow);
    }

private:
    /**
     * @brief Weryfikuje czy przekazany bufor nie jest pusty.
     * @param data Bufor do sprawdzenia.
     * @return PacketResult<std::span<const uint8_t>> przekazany bufor lub blad.
     */
    [[nodiscard]] static PacketResult<std::span<const uint8_t>> ValidateSpan(std::span<const uint8_t> data) noexcept {
        if (data.empty()) {
            ModernLogger::Error("SIMD_CRC32Hardware::ValidateSpan: Buffer is empty.");
            return MakeError(PacketError::BufferUnderflow);
        }
        return data;
    }

    /**
     * @brief Wewnetrzna implementacja obliczania CRC32 wykorzystujaca instrukcje x64.
     * @param data Zweryfikowany bufor danych.
     * @param initialCrc Poczatkowa suma kontrolna.
     * @return Obliczona suma CRC32.
     */
    [[nodiscard]] static uint32_t ComputeInternal(std::span<const uint8_t> data, uint32_t initialCrc) noexcept {
        uint32_t crc = initialCrc;
        size_t size = data.size();
        const uint8_t* ptr = data.data();

#if defined(_M_X64) || defined(__x86_64__)
        // Przetwarzanie po 8 bajtow
        while (size >= 8) {
            uint64_t qword;
            std::memcpy(&qword, ptr, sizeof(uint64_t));
            crc = static_cast<uint32_t>(_mm_crc32_u64(crc, qword));
            ptr += 8;
            size -= 8;
        }

        // Przetwarzanie po 4 bajty
        if (size >= 4) {
            uint32_t dword;
            std::memcpy(&dword, ptr, sizeof(uint32_t));
            crc = _mm_crc32_u32(crc, dword);
            ptr += 4;
            size -= 4;
        }

        // Przetwarzanie po 2 bajty
        if (size >= 2) {
            uint16_t word;
            std::memcpy(&word, ptr, sizeof(uint16_t));
            crc = _mm_crc32_u16(crc, word);
            ptr += 2;
            size -= 2;
        }

        // Przetwarzanie pojedynczych bajtow
        while (size > 0) {
            crc = _mm_crc32_u8(crc, *ptr);
            ptr++;
            size--;
        }
#else
        ModernLogger::Warn("SIMD_CRC32Hardware: SSE4.2 nie jest wsprane na tej architekturze. Zwracanie niezmienionego CRC.");
#endif

        crc ^= 0xFFFFFFFF; // Finalizacja XOR

        // Publikacja zdarzenia w celach informacyjnych, calkowicie odpieta od GUI
        UserInterface::Core::EventBus::GetInstance().Publish(HardwareCrc32CalculatedEvent{crc, data.size()});

        return crc;
    }
};

} // namespace EterBase
