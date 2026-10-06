#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <string_view>
#include <cstring>
#include <type_traits>

/**
 * @class ByteBuffer
 * @brief A safe FIFO byte buffer for network packets with automatic memory rolling.
 * 
 * This class implements a dynamic byte buffer that manages memory automatically, 
 * expanding when necessary and rolling memory back to the beginning when space 
 * is freed from the front. It uses modern C++20 standard types like std::span 
 * for safety.
 */
class ByteBuffer {
public:
    /**
     * @brief Constructs a new ByteBuffer with a specified initial capacity.
     * @param capacity The initial capacity of the buffer in bytes. Defaults to 4096.
     */
    explicit ByteBuffer(uint32_t capacity = 4096)
        : buffer(capacity), read_position(0), write_position(0) {}

    /**
     * @brief Writes data to the end of the buffer.
     * @param data A span of bytes to write into the buffer.
     * @return True if the write was successful, false otherwise.
     */
    bool Write(std::span<const uint8_t> data) {
        if (data.empty()) {
            return true;
        }

        EnsureCapacity(data.size());

        std::memcpy(buffer.data() + write_position, data.data(), data.size());
        write_position += data.size();

        return true;
    }
    
    /**
     * @brief Writes an arbitrary trivially copyable object to the buffer.
     * @tparam T The type of the object to write.
     * @param value The object to write.
     * @return True if successful.
     */
    template <typename T>
    bool WriteValue(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable.");
        std::span<const uint8_t> data_span(reinterpret_cast<const uint8_t*>(&value), sizeof(T));
        return Write(data_span);
    }

    /**
     * @brief Reads data from the front of the buffer into a provided span.
     * @param out_data A span of bytes where the read data will be stored.
     * @return True if there was enough data to read, false otherwise.
     */
    bool Read(std::span<uint8_t> out_data) {
        if (out_data.size() > GetActiveSize()) {
            return false;
        }

        std::memcpy(out_data.data(), buffer.data() + read_position, out_data.size());
        read_position += out_data.size();

        return true;
    }

    /**
     * @brief Reads an arbitrary trivially copyable object from the buffer.
     * @tparam T The type of the object to read.
     * @param out_value The object to read into.
     * @return True if successful, false if not enough data.
     */
    template <typename T>
    bool ReadValue(T& out_value) {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable.");
        std::span<uint8_t> data_span(reinterpret_cast<uint8_t*>(&out_value), sizeof(T));
        return Read(data_span);
    }

    /**
     * @brief Peeks at data from the front of the buffer without advancing the read position.
     * @param out_data A span of bytes where the peeked data will be stored.
     * @return True if there was enough data to peek, false otherwise.
     */
    bool Peek(std::span<uint8_t> out_data) const {
        if (out_data.size() > GetActiveSize()) {
            return false;
        }

        std::memcpy(out_data.data(), buffer.data() + read_position, out_data.size());
        return true;
    }

    /**
     * @brief Gets the number of bytes currently available to read.
     * @return The number of bytes that can be read.
     */
    uint32_t GetActiveSize() const {
        return write_position - read_position;
    }

    /**
     * @brief Gets the total capacity of the buffer.
     * @return The capacity in bytes.
     */
    uint32_t GetCapacity() const {
        return static_cast<uint32_t>(buffer.size());
    }

    /**
     * @brief Clears the buffer, resetting read and write positions to zero.
     */
    void Clear() {
        read_position = 0;
        write_position = 0;
    }

    /**
     * @brief Retrieves a span representing the currently active readable data.
     * @return A constant span of the readable bytes.
     */
    std::span<const uint8_t> GetReadSpan() const {
        return {buffer.data() + read_position, GetActiveSize()};
    }

private:
    /**
     * @brief Ensures that there is enough capacity to write the specified number of bytes.
     * 
     * If there is not enough space at the end of the buffer, it will attempt to roll
     * existing active data to the beginning of the buffer. If that is still not enough,
     * it will resize the buffer.
     * 
     * @param size_to_add The number of bytes to accommodate.
     */
    void EnsureCapacity(uint32_t size_to_add) {
        uint32_t active_size = GetActiveSize();
        uint32_t remaining_space = static_cast<uint32_t>(buffer.size()) - write_position;

        if (remaining_space >= size_to_add) {
            return;
        }

        // If rolling the data to the front provides enough space
        if (static_cast<uint32_t>(buffer.size()) - active_size >= size_to_add) {
            std::memmove(buffer.data(), buffer.data() + read_position, active_size);
            read_position = 0;
            write_position = active_size;
        } else {
            // Otherwise, we need to expand the buffer
            // Move data to the front if read_position > 0 to save space
            if (read_position > 0) {
                std::memmove(buffer.data(), buffer.data() + read_position, active_size);
                read_position = 0;
                write_position = active_size;
            }
            
            uint32_t new_capacity = static_cast<uint32_t>(buffer.size()) * 2;
            if (new_capacity < active_size + size_to_add) {
                new_capacity = active_size + size_to_add;
            }
            buffer.resize(new_capacity);
        }
    }

    std::vector<uint8_t> buffer;
    uint32_t read_position;
    uint32_t write_position;
};
