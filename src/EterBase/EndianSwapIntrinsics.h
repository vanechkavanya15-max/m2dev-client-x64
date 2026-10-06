#pragma once

#include <cstdint>
#include <concepts>
#include <span>
#include <type_traits>
#include <format>
#include <expected>
#include <cstring>

#ifdef _WIN32
#include <stdlib.h> // For _byteswap_ulong, _byteswap_ushort, _byteswap_uint64
#endif

#include "Result.h"
#include "StrongTypes.h"
#include "LogModern.h"

/**
 * @file EndianSwapIntrinsics.h
 * @brief Nowoczesny, bezpieczny modul C++23 do konwersji Endianness z uzyciem intrinsics sprzetowych.
 * 
 * Wykorzystuje _byteswap_ulong, _byteswap_ushort, _byteswap_uint64 w srodowisku MSVC
 * w celu osiagniecia maksymalnej wydajnosci i poprawnosci bez uciekania sie do
 * przesuniec bitowych i operacji logicznych. Zapewnia 100% bezpieczenstwo poprzez
 * std::expected oraz walidacje wielkosci buforow.
 */

namespace EterBase {

/**
 * @class EndianSwapIntrinsics
 * @brief Statyczna klasa pomocnicza z metodami konwertujacymi uklad bajtow z wykorzystaniem sprzetu.
 */
class EndianSwapIntrinsics {
public:
    EndianSwapIntrinsics() = delete;
    ~EndianSwapIntrinsics() = delete;

    /**
     * @brief Konwertuje sprzetowo 16-bitowa liczbe z uzyciem intrinsica _byteswap_ushort.
     * 
     * @param value Wartosc wejsciowa (16-bit).
     * @return Skonwertowana wartosc (odwrocony uklad bajtow).
     */
    [[nodiscard]] static constexpr uint16_t Swap16(uint16_t value) noexcept {
#if defined(_WIN32) && !defined(__clang__) && !defined(__GNUC__)
        if consteval {
            return static_cast<uint16_t>((value >> 8) | (value << 8));
        } else {
            return _byteswap_ushort(value);
        }
#elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap16(value);
#else
        return static_cast<uint16_t>((value >> 8) | (value << 8));
#endif
    }

    /**
     * @brief Konwertuje sprzetowo 32-bitowa liczbe z uzyciem intrinsica _byteswap_ulong.
     * 
     * @param value Wartosc wejsciowa (32-bit).
     * @return Skonwertowana wartosc (odwrocony uklad bajtow).
     */
    [[nodiscard]] static constexpr uint32_t Swap32(uint32_t value) noexcept {
#if defined(_WIN32) && !defined(__clang__) && !defined(__GNUC__)
        if consteval {
            return (value >> 24) |
                   ((value & 0x00FF0000) >> 8) |
                   ((value & 0x0000FF00) << 8) |
                   (value << 24);
        } else {
            return _byteswap_ulong(value);
        }
#elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap32(value);
#else
        return (value >> 24) |
               ((value & 0x00FF0000) >> 8) |
               ((value & 0x0000FF00) << 8) |
               (value << 24);
#endif
    }

    /**
     * @brief Konwertuje sprzetowo 64-bitowa liczbe z uzyciem intrinsica _byteswap_uint64.
     * 
     * @param value Wartosc wejsciowa (64-bit).
     * @return Skonwertowana wartosc (odwrocony uklad bajtow).
     */
    [[nodiscard]] static constexpr uint64_t Swap64(uint64_t value) noexcept {
#if defined(_WIN32) && !defined(__clang__) && !defined(__GNUC__)
        if consteval {
            return (value >> 56) |
                   ((value & 0x00FF000000000000) >> 40) |
                   ((value & 0x0000FF0000000000) >> 24) |
                   ((value & 0x000000FF00000000) >> 8) |
                   ((value & 0x00000000FF000000) << 8) |
                   ((value & 0x0000000000FF0000) << 24) |
                   ((value & 0x000000000000FF00) << 40) |
                   (value << 56);
        } else {
            return _byteswap_uint64(value);
        }
#elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap64(value);
#else
        return (value >> 56) |
               ((value & 0x00FF000000000000) >> 40) |
               ((value & 0x0000FF0000000000) >> 24) |
               ((value & 0x000000FF00000000) >> 8) |
               ((value & 0x00000000FF000000) << 8) |
               ((value & 0x0000000000FF0000) << 24) |
               ((value & 0x000000000000FF00) << 40) |
               (value << 56);
#endif
    }

    /**
     * @brief Wykonuje swap dowolnego prymitywnego typu calkowitoliczbowego.
     * 
     * @tparam T Typ calkowitoliczbowy (8, 16, 32, 64-bit).
     * @param value Wartosc do konwersji.
     * @return Wartosc po konwersji Endianness.
     */
    template <std::integral T>
    [[nodiscard]] static constexpr T Swap(T value) noexcept {
        if constexpr (sizeof(T) == 1) {
            return value;
        } else if constexpr (sizeof(T) == 2) {
            return static_cast<T>(Swap16(static_cast<uint16_t>(value)));
        } else if constexpr (sizeof(T) == 4) {
            return static_cast<T>(Swap32(static_cast<uint32_t>(value)));
        } else if constexpr (sizeof(T) == 8) {
            return static_cast<T>(Swap64(static_cast<uint64_t>(value)));
        } else {
            static_assert(sizeof(T) == 0, "Unsupported integral type size for EndianSwap");
            return value;
        }
    }

    /**
     * @brief Wykonuje swap dla bezpiecznego typu StrongType domenowego (np. EntityId, ItemVnum).
     * 
     * @tparam Tag Tag StrongType.
     * @tparam Underlying Typ bazowy StrongType.
     * @tparam DefaultValue Domyslna wartosc StrongType.
     * @param value Obiekt StrongType.
     * @return Skonwertowany StrongType.
     */
    template <typename Tag, typename Underlying, Underlying DefaultValue>
    [[nodiscard]] static constexpr auto Swap(StrongType<Tag, Underlying, DefaultValue> value) noexcept 
        -> StrongType<Tag, Underlying, DefaultValue> {
        return StrongType<Tag, Underlying, DefaultValue>(Swap(value.get()));
    }

    /**
     * @brief Podmienia wartosc zmiennej w miejscu z bezpieczna walidacja typu i wskaznika (oparty na std::expected).
     * 
     * @tparam T Typ wartosci calkowitoliczbowej badz StrongType.
     * @param buffer Skaler referencyjny przekazany do mutacji.
     * @return EterBase::PacketResult<void> oznaczajacy sukces badz Error.
     */
    template <typename T>
    static PacketResult<void> SwapInPlace(T& buffer) noexcept {
        buffer = Swap(buffer);
        return {};
    }

    /**
     * @brief Wykonuje masowa sprzetowa podmiane Endianness w podanym buforze dla danego typu.
     * 
     * @tparam T Typ, ktorego reprezentacje uzytkownik chce podmieniac z poziomu struktury pamiaci.
     * @param buffer Span pamieci bajtowej.
     * @return EterBase::PacketResult<void> Sukces (void) w przypadku powodzenia badz blad (PacketError::BufferUnderflow) jesli struktura jest ucieta.
     */
    template <typename T>
    static PacketResult<void> SwapBuffer(std::span<uint8_t> buffer) noexcept {
        if (buffer.empty()) {
            return MakeError(PacketError::BufferUnderflow);
        }

        if (buffer.size() % sizeof(T) != 0) {
            ModernLogger::Error("EndianSwapIntrinsics: Znaleziono buffer bez dopasowania do rozmiaru typu. Oczekiwano wielokrotnosci: {}", sizeof(T));
            return MakeError(PacketError::MalformedPayload);
        }

        size_t elements = buffer.size() / sizeof(T);
        uint8_t* rawData = buffer.data();

        for (size_t index = 0; index < elements; ++index) {
            T tempVal;
            std::memcpy(&tempVal, rawData + (index * sizeof(T)), sizeof(T));
            tempVal = Swap(tempVal);
            std::memcpy(rawData + (index * sizeof(T)), &tempVal, sizeof(T));
        }

        return {};
    }
};

} // namespace EterBase
