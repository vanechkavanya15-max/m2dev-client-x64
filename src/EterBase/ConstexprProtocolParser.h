#pragma once

#include "Result.h"
#include "LogModern.h"
#include "StrongTypes.h"
#include <cstdint>
#include <span>
#include <expected>
#include <type_traits>
#include <format>
#include <optional>

/**
 * @file ConstexprProtocolParser.h
 * @brief Nowoczesny parser protokolow sieciowych z walidacja w czasie kompilacji (C++23).
 */

namespace EterBase {

/**
 * @class ConstexprProtocolParser
 * @brief Narzedzie do bezpiecznego parsowania pakietow sieciowych.
 * 
 * Klasa zapewnia statyczna asercje rozmiaru struktury pakietu (static_assert)
 * w celu wczesnego wykrywania niezgodnosci binarnej. Wykorzystuje std::span
 * do bezpiecznego odczytu z buforow w pamieci, gwarantujac brak przepelnienia bufora.
 */
class ConstexprProtocolParser {
public:
    /**
     * @brief Odczytuje pakiet z bufora sprawdzajac jego poprawnosc.
     * 
     * Wykonuje walidacje rozmiaru bufora w czasie wykonywania oraz
     * walidacje definicji pakietu w czasie kompilacji (static_assert).
     * 
     * @tparam TPacket Typ struktury pakietu.
     * @tparam ExpectedSize Oczekiwany rozmiar struktury pakietu w bajtach.
     * @param buffer Bufor z danymi sieciowymi (span).
     * @return EterBase::PacketResult<const TPacket*> Wskaznik do zrzutowanego pakietu w przypadku sukcesu lub kod bledu w przypadku porazki.
     */
    template <typename TPacket, size_t ExpectedSize>
    [[nodiscard]] static PacketResult<const TPacket*> Parse(std::span<const uint8_t> buffer) noexcept {
        static_assert(std::is_trivially_copyable_v<TPacket>, "Packet must be trivially copyable.");
        static_assert(sizeof(TPacket) == ExpectedSize, "Packet structure size does not match expected size.");

        if (buffer.size() < ExpectedSize) {
            ModernLogger::Log(LogLevel::Error, "Buffer underflow: expected {} bytes, got {} bytes.", ExpectedSize, buffer.size());
            return MakeError(PacketError::BufferUnderflow);
        }

        // Wskaznik odczytywany w sposob bezpieczny
        const TPacket* packet = reinterpret_cast<const TPacket*>(buffer.data());
        return packet;
    }

    /**
     * @brief Zwraca pakiet jako std::optional, integrujac sie z monadycznymi operacjami (transform, and_then).
     * 
     * Zamiast wielopoziomowych instrukcji if, obsluga bledow mapowana jest na std::nullopt.
     * 
     * @tparam TPacket Typ struktury pakietu.
     * @tparam ExpectedSize Oczekiwany rozmiar struktury pakietu w bajtach.
     * @param buffer Bufor z danymi sieciowymi.
     * @return std::optional<const TPacket*> Zdekodowany pakiet lub wartosc pusta.
     */
    template <typename TPacket, size_t ExpectedSize>
    [[nodiscard]] static std::optional<const TPacket*> ParseOptional(std::span<const uint8_t> buffer) noexcept {
        auto result = Parse<TPacket, ExpectedSize>(buffer);
        if (result.has_value()) {
            return result.value();
        }
        return std::nullopt;
    }

    /**
     * @brief Odczytuje pakiet z przesunieciem, weryfikujac dostepne dane w buforze.
     * 
     * Przydatne dla sekwencyjnego przetwarzania wielu danych po sobie (np. dynamiczne payloady).
     * Funkcja modyfikuje przekazany offset, jesli pakiet zostal pomyslnie sparsowany.
     * 
     * @tparam TPacket Typ struktury pakietu.
     * @tparam ExpectedSize Oczekiwany rozmiar struktury pakietu w bajtach.
     * @param buffer Bufor zawierajacy pakiet.
     * @param offset Referencja na wskaznik offsetu do zaktualizowania przy pomyslnym odczycie.
     * @return EterBase::PacketResult<const TPacket*> Wczytany wskaznik na pakiet albo kod bledu w formacie Result C++23.
     */
    template <typename TPacket, size_t ExpectedSize>
    [[nodiscard]] static PacketResult<const TPacket*> ParseWithOffset(std::span<const uint8_t> buffer, size_t& offset) noexcept {
        if (offset > buffer.size()) {
            ModernLogger::Log(LogLevel::Error, "Offset out of bounds: {} > {}", offset, buffer.size());
            return MakeError(PacketError::BufferUnderflow);
        }

        auto subBuffer = buffer.subspan(offset);
        auto result = Parse<TPacket, ExpectedSize>(subBuffer);

        if (result.has_value()) {
            offset += ExpectedSize;
        }

        return result;
    }
};

} // namespace EterBase
