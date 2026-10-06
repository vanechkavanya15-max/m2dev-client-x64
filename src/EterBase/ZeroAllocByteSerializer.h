#pragma once

#include <span>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <string_view>
#include <algorithm>
#include <limits>

#include "Result.h"
#include "StrongTypes.h"
#include "LogModern.h"

namespace EterBase {

/**
 * @class ZeroAllocByteSerializer
 * @brief Binary serializer writing directly to std::span without allocations.
 * 
 * Supports serializing standard primitive types, EterBase::StrongType, and strings
 * into a pre-allocated memory buffer. Ensures memory safety using bounds checking.
 * Employs std::expected<T, E> for error handling.
 */
class ZeroAllocByteSerializer {
public:
    /**
     * @brief Constructs the serializer with a target buffer.
     * @param buffer The destination span where data will be serialized.
     */
    explicit constexpr ZeroAllocByteSerializer(std::span<uint8_t> buffer) noexcept
        : m_buffer(buffer), m_offset(0) {}

    /**
     * @brief Serializes a trivial type (e.g., primitives, structs).
     * @tparam T The type of the value to serialize. Must be trivially copyable.
     * @param value The value to serialize.
     * @return PacketResult<void> Returns success or BufferUnderflow error if the buffer is too small.
     */
    template <typename T>
    requires std::is_trivially_copyable_v<T>
    [[nodiscard]] PacketResult<void> Write(const T& value) noexcept {
        if (sizeof(T) > m_buffer.size() - m_offset) {
            ModernLogger::Error("ZeroAllocByteSerializer: Buffer capacity exceeded while writing type of size {}", sizeof(T));
            return MakeError(PacketError::BufferUnderflow);
        }

        std::memcpy(m_buffer.data() + m_offset, &value, sizeof(T));
        m_offset += sizeof(T);

        return {};
    }

    /**
     * @brief Serializes a EterBase::StrongType.
     * @tparam Tag The tag of the StrongType.
     * @tparam Underlying The underlying type.
     * @tparam DefaultValue The default value.
     * @param value The strong type value to serialize.
     * @return PacketResult<void> Returns success or BufferUnderflow error if the buffer is too small.
     */
    template <typename Tag, typename Underlying, Underlying DefaultValue>
    [[nodiscard]] PacketResult<void> Write(const StrongType<Tag, Underlying, DefaultValue>& value) noexcept {
        return Write(value.value());
    }

    /**
     * @brief Serializes a sequence of bytes.
     * @param data The span of bytes to serialize.
     * @return PacketResult<void> Returns success or BufferUnderflow error if the buffer is too small.
     */
    [[nodiscard]] PacketResult<void> WriteBytes(std::span<const uint8_t> data) noexcept {
        if (data.size() > m_buffer.size() - m_offset) {
            ModernLogger::Error("ZeroAllocByteSerializer: Buffer capacity exceeded while writing {} bytes", data.size());
            return MakeError(PacketError::BufferUnderflow);
        }

        if (!data.empty()) {
            std::memcpy(m_buffer.data() + m_offset, data.data(), data.size());
            m_offset += data.size();
        }

        return {};
    }

    /**
     * @brief Serializes a string with a fixed maximum length, padded with null bytes.
     * @param str The string view to serialize.
     * @param maxLength The exact number of bytes the string will occupy in the buffer.
     * @return PacketResult<void> Returns success or BufferUnderflow error if the buffer is too small.
     */
    [[nodiscard]] PacketResult<void> WriteStringFixed(std::string_view str, size_t maxLength) noexcept {
        if (maxLength > m_buffer.size() - m_offset) {
            ModernLogger::Error("ZeroAllocByteSerializer: Buffer capacity exceeded while writing fixed string of size {}", maxLength);
            return MakeError(PacketError::BufferUnderflow);
        }

        size_t writeLen = std::min(str.size(), maxLength);
        if (writeLen > 0) {
            std::memcpy(m_buffer.data() + m_offset, str.data(), writeLen);
        }
        
        // Pad with zeros
        if (writeLen < maxLength) {
            std::memset(m_buffer.data() + m_offset + writeLen, 0, maxLength - writeLen);
        }

        m_offset += maxLength;
        return {};
    }

    /**
     * @brief Serializes a string with its length prepended.
     * @tparam LenType The type used to store the length (e.g., uint16_t).
     * @param str The string view to serialize.
     * @return PacketResult<void> Returns success or an error if the buffer is too small or the string is too long.
     */
    template <typename LenType = uint16_t>
    requires std::is_integral_v<LenType>
    [[nodiscard]] PacketResult<void> WriteString(std::string_view str) noexcept {
        if (str.size() > std::numeric_limits<LenType>::max()) {
            ModernLogger::Error("ZeroAllocByteSerializer: String length ({}) exceeds maximum allowed by length type ({})", str.size(), std::numeric_limits<LenType>::max());
            return MakeError(PacketError::MalformedPayload);
        }

        return Write(static_cast<LenType>(str.size()))
            .and_then([this, str]() {
                return WriteBytes(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(str.data()), str.size()));
            });
    }

    /**
     * @brief Gets the number of bytes written so far.
     * @return The number of bytes written.
     */
    [[nodiscard]] constexpr size_t GetBytesWritten() const noexcept {
        return m_offset;
    }

    /**
     * @brief Gets the span of the serialized data.
     * @return A span representing the written portion of the buffer.
     */
    [[nodiscard]] constexpr std::span<const uint8_t> GetWrittenData() const noexcept {
        return m_buffer.first(m_offset);
    }

    /**
     * @brief Resets the serializer to the beginning of the buffer.
     */
    constexpr void Reset() noexcept {
        m_offset = 0;
    }

private:
    std::span<uint8_t> m_buffer;
    size_t m_offset;
};

} // namespace EterBase
