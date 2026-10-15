#ifndef ETERBASE_ENDIAN_HELPER_H
#define ETERBASE_ENDIAN_HELPER_H

#include <cstdint>
#include <bit>
#include <concepts>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <algorithm>

namespace EterBase::EndianHelper
{

    /**
     * @brief Swaps the byte order of an integral value.
     * 
     * @tparam T The type of the value to swap. Must be an integral type.
     * @param value The value whose bytes to swap.
     * @return The value with its byte order swapped.
     */
    template <std::integral T>
    [[nodiscard]] constexpr T SwapBytes(T value) noexcept
    {
        if constexpr (sizeof(T) == 1) {
            return value;
        } else if constexpr (sizeof(T) == 2) {
            return static_cast<T>((static_cast<uint16_t>(value) >> 8) |
                                  (static_cast<uint16_t>(value) << 8));
        } else if constexpr (sizeof(T) == 4) {
            return static_cast<T>((static_cast<uint32_t>(value) >> 24) |
                                  ((static_cast<uint32_t>(value) & 0x00FF0000) >> 8) |
                                  ((static_cast<uint32_t>(value) & 0x0000FF00) << 8) |
                                  (static_cast<uint32_t>(value) << 24));
        } else if constexpr (sizeof(T) == 8) {
            return static_cast<T>((static_cast<uint64_t>(value) >> 56) |
                                  ((static_cast<uint64_t>(value) & 0x00FF000000000000) >> 40) |
                                  ((static_cast<uint64_t>(value) & 0x0000FF0000000000) >> 24) |
                                  ((static_cast<uint64_t>(value) & 0x000000FF00000000) >> 8) |
                                  ((static_cast<uint64_t>(value) & 0x00000000FF000000) << 8) |
                                  ((static_cast<uint64_t>(value) & 0x0000000000FF0000) << 24) |
                                  ((static_cast<uint64_t>(value) & 0x000000000000FF00) << 40) |
                                  (static_cast<uint64_t>(value) << 56));
        }
    }

    /**
     * @brief Converts a value from native endianness to little-endian.
     * 
     * @tparam T The type of the value to convert. Must be an integral type.
     * @param value The value in native endianness.
     * @return The value converted to little-endian.
     */
    template <std::integral T>
    [[nodiscard]] constexpr T ToLittleEndian(T value) noexcept
    {
        if constexpr (std::endian::native == std::endian::little) {
            return value;
        } else {
            return SwapBytes(value);
        }
    }

    /**
     * @brief Converts a value from little-endian to native endianness.
     * 
     * @tparam T The type of the value to convert. Must be an integral type.
     * @param value The little-endian value.
     * @return The value converted to native endianness.
     */
    template <std::integral T>
    [[nodiscard]] constexpr T FromLittleEndian(T value) noexcept
    {
        return ToLittleEndian(value);
    }

    /**
     * @brief Converts a value from native endianness to big-endian.
     * 
     * @tparam T The type of the value to convert. Must be an integral type.
     * @param value The value in native endianness.
     * @return The value converted to big-endian.
     */
    template <std::integral T>
    [[nodiscard]] constexpr T ToBigEndian(T value) noexcept
    {
        if constexpr (std::endian::native == std::endian::big) {
            return value;
        } else {
            return SwapBytes(value);
        }
    }

    /**
     * @brief Converts a value from big-endian to native endianness.
     * 
     * @tparam T The type of the value to convert. Must be an integral type.
     * @param value The big-endian value.
     * @return The value converted to native endianness.
     */
    template <std::integral T>
    [[nodiscard]] constexpr T FromBigEndian(T value) noexcept
    {
        return ToBigEndian(value);
    }

    /**
     * @brief Safely reads a little-endian integer from a span of bytes.
     * 
     * @tparam T The type of the value to read. Must be an integral type.
     * @param buffer The span of bytes to read from.
     * @param offset The byte offset to start reading. Defaults to 0.
     * @return The integer value in native endianness.
     * @throws std::out_of_range if the buffer is too small to contain the value at the given offset.
     */
    template <std::integral T>
    [[nodiscard]] inline T ReadLittleEndian(std::span<const uint8_t> buffer, size_t offset = 0)
    {
        if (offset + sizeof(T) > buffer.size()) {
            throw std::out_of_range("Buffer too small to read little-endian value");
        }
        T rawValue{};
        std::copy_n(buffer.data() + offset, sizeof(T), reinterpret_cast<uint8_t*>(&rawValue));
        return FromLittleEndian(rawValue);
    }

    /**
     * @brief Safely reads a big-endian integer from a span of bytes.
     * 
     * @tparam T The type of the value to read. Must be an integral type.
     * @param buffer The span of bytes to read from.
     * @param offset The byte offset to start reading. Defaults to 0.
     * @return The integer value in native endianness.
     * @throws std::out_of_range if the buffer is too small to contain the value at the given offset.
     */
    template <std::integral T>
    [[nodiscard]] inline T ReadBigEndian(std::span<const uint8_t> buffer, size_t offset = 0)
    {
        if (offset + sizeof(T) > buffer.size()) {
            throw std::out_of_range("Buffer too small to read big-endian value");
        }
        T rawValue{};
        std::copy_n(buffer.data() + offset, sizeof(T), reinterpret_cast<uint8_t*>(&rawValue));
        return FromBigEndian(rawValue);
    }

    /**
     * @brief Safely writes a little-endian integer to a span of bytes.
     * 
     * @tparam T The type of the value to write. Must be an integral type.
     * @param buffer The span of bytes to write to.
     * @param value The value to write in native endianness.
     * @param offset The byte offset to start writing. Defaults to 0.
     * @throws std::out_of_range if the buffer is too small to contain the value at the given offset.
     */
    template <std::integral T>
    inline void WriteLittleEndian(std::span<uint8_t> buffer, T value, size_t offset = 0)
    {
        if (offset + sizeof(T) > buffer.size()) {
            throw std::out_of_range("Buffer too small to write little-endian value");
        }
        T endianValue = ToLittleEndian(value);
        std::copy_n(reinterpret_cast<const uint8_t*>(&endianValue), sizeof(T), buffer.data() + offset);
    }

    /**
     * @brief Safely writes a big-endian integer to a span of bytes.
     * 
     * @tparam T The type of the value to write. Must be an integral type.
     * @param buffer The span of bytes to write to.
     * @param value The value to write in native endianness.
     * @param offset The byte offset to start writing. Defaults to 0.
     * @throws std::out_of_range if the buffer is too small to contain the value at the given offset.
     */
    template <std::integral T>
    inline void WriteBigEndian(std::span<uint8_t> buffer, T value, size_t offset = 0)
    {
        if (offset + sizeof(T) > buffer.size()) {
            throw std::out_of_range("Buffer too small to write big-endian value");
        }
        T endianValue = ToBigEndian(value);
        std::copy_n(reinterpret_cast<const uint8_t*>(&endianValue), sizeof(T), buffer.data() + offset);
    }

} // namespace EterBase::EndianHelper

#endif // ETERBASE_ENDIAN_HELPER_H
