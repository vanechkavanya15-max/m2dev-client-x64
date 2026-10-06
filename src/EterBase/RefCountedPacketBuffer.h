#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <cstring>
#include <optional>
#include <type_traits>

#include "Result.h"
#include "StrongTypes.h"
#include "LogModern.h"
namespace EterBase {

/**
 * @class RefCountedPacketBuffer
 * @brief A modern, C++23 zero-copy reference-counted memory buffer for network packets.
 * 
 * Manages an immutable block of memory via std::shared_ptr, allowing it to be
 * safely passed around or sliced without copying the underlying bytes. It provides
 * safe extraction of primitive types and strongly-typed entities using std::span
 * and std::expected based PacketResult.
 */
class RefCountedPacketBuffer {
public:
    /**
     * @brief Constructs a new RefCountedPacketBuffer with the given capacity.
     * @param capacity The number of bytes to allocate.
     */
    explicit RefCountedPacketBuffer(uint32_t capacity)
        : m_capacity(capacity),
          m_buffer(std::make_shared<uint8_t[]>(capacity)),
          m_readOffset(0),
          m_writeOffset(0) {}

    /**
     * @brief Constructs a new buffer by copying data from a span.
     * @param data A span containing the source data.
     */
    explicit RefCountedPacketBuffer(std::span<const uint8_t> data)
        : m_capacity(static_cast<uint32_t>(data.size())),
          m_buffer(std::make_shared<uint8_t[]>(data.size())),
          m_readOffset(0),
          m_writeOffset(static_cast<uint32_t>(data.size())) {
        std::memcpy(m_buffer.get(), data.data(), data.size());
    }

    /**
     * @brief Returns a read-only view of the currently active (unread) data.
     * @return A std::span covering the unread portion of the buffer.
     */
    [[nodiscard]] std::span<const uint8_t> GetSpan() const noexcept {
        return {m_buffer.get() + m_readOffset, m_writeOffset - m_readOffset};
    }

    /**
     * @brief Returns a mutable view of the remaining writable space.
     * @return A std::span covering the writable portion of the buffer.
     */
    [[nodiscard]] std::span<uint8_t> GetMutableSpan() noexcept {
        return {m_buffer.get() + m_writeOffset, m_capacity - m_writeOffset};
    }

    /**
     * @brief Reads a trivially copyable type from the buffer.
     * @tparam T The type to read.
     * @return A PacketResult containing the parsed value on success, or a PacketError on failure.
     */
    template <typename T>
    [[nodiscard]] PacketResult<T> ReadValue() noexcept {
        static_assert(std::is_trivially_copyable_v<T>, "ReadValue requires a trivially copyable type");

        if (GetSpan().size_bytes() < sizeof(T)) {
            ModernLogger::Error("Buffer underflow when trying to read {} bytes", sizeof(T));
            return MakeError(PacketError::BufferUnderflow);
        }

        T value;
        std::memcpy(&value, m_buffer.get() + m_readOffset, sizeof(T));
        m_readOffset += sizeof(T);
        return value;
    }

    /**
     * @brief Reads into a provided object reference.
     * @tparam T The type of the object.
     * @param out_value Reference to the object to populate.
     * @return A VoidResult indicating success or failure.
     */
    template <typename T>
    [[nodiscard]] PacketResult<void> ReadInto(T& out_value) noexcept {
        static_assert(std::is_trivially_copyable_v<T>, "ReadInto requires a trivially copyable type");

        if (GetSpan().size_bytes() < sizeof(T)) {
            ModernLogger::Error("Buffer underflow when trying to read into {} bytes", sizeof(T));
            return MakeError(PacketError::BufferUnderflow);
        }

        std::memcpy(&out_value, m_buffer.get() + m_readOffset, sizeof(T));
        m_readOffset += sizeof(T);
        return {};
    }

    /**
     * @brief Reads an EntityId safely using monadic expected/optional operations.
     * @return An optional containing the EntityId if successful, std::nullopt otherwise.
     */
    [[nodiscard]] std::optional<EntityId> ReadEntityId() noexcept {
        auto expected = ReadValue<uint32_t>().transform([](uint32_t val) { return EntityId{val}; });
        if (expected) {
            return *expected;
        }
        return std::nullopt;
    }
    
    /**
     * @brief Advances the write offset by a specified amount (e.g., after direct span writes).
     * @param amount The number of bytes to advance.
     * @return True if successful, false if it exceeds capacity.
     */
    bool AdvanceWrite(uint32_t amount) noexcept {
        if (m_writeOffset + amount > m_capacity) {
             return false;
        }
        m_writeOffset += amount;
        return true;
    }

private:
    uint32_t m_capacity;
    std::shared_ptr<uint8_t[]> m_buffer;
    uint32_t m_readOffset;
    uint32_t m_writeOffset;
};

} // namespace EterBase
