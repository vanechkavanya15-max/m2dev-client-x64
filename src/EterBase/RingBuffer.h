#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <array>
#include <algorithm>

namespace EterBase {

/**
 * @brief An allocation-free ring buffer designed for TCP socket packet streams.
 *
 * This class pre-allocates a fixed capacity buffer and provides read/write
 * operations using a wrap-around circular logic. It avoids dynamic allocations
 * during runtime.
 */
class RingBuffer {
public:
    /**
     * @brief Constructs a new RingBuffer with a specified capacity.
     * @param capacity The fixed capacity of the buffer in bytes.
     */
    explicit RingBuffer(size_t capacity)
        : buffer_(std::max<size_t>(1, capacity)), 
          capacity_(std::max<size_t>(1, capacity)), 
          head_(0), tail_(0), size_(0) {}

    /**
     * @brief Writes data into the buffer.
     * @param data A span representing the binary data to write.
     * @return True if the entire data was successfully written, false if there is not enough space.
     */
    bool Write(std::span<const uint8_t> data) {
        if (data.size() > GetFreeSpace()) {
            return false;
        }

        size_t first_part = std::min(data.size(), capacity_ - tail_);
        std::copy_n(data.data(), first_part, buffer_.data() + tail_);
        
        if (first_part < data.size()) {
            std::copy_n(data.data() + first_part, data.size() - first_part, buffer_.data());
        }
        
        tail_ = (tail_ + data.size()) % capacity_;
        size_ += data.size();
        return true;
    }

    /**
     * @brief Reads data from the buffer and removes it.
     * @param dest A span representing the destination buffer to write the read data to.
     * @return True if the requested amount of data was successfully read, false if not enough data is available.
     */
    bool Read(std::span<uint8_t> dest) {
        if (Peek(dest)) {
            Skip(dest.size());
            return true;
        }
        return false;
    }

    /**
     * @brief Peeks at data from the buffer without removing it.
     * @param dest A span representing the destination buffer to write the peeked data to.
     * @return True if the requested amount of data was successfully peeked, false if not enough data is available.
     */
    bool Peek(std::span<uint8_t> dest) const {
        if (dest.size() > size_) {
            return false;
        }

        size_t first_part = std::min(dest.size(), capacity_ - head_);
        std::copy_n(buffer_.data() + head_, first_part, dest.data());
        
        if (first_part < dest.size()) {
            std::copy_n(buffer_.data(), dest.size() - first_part, dest.data() + first_part);
        }
        
        return true;
    }

    /**
     * @brief Skips a specified number of bytes in the buffer.
     * @param count The number of bytes to skip.
     * @return True if the bytes were successfully skipped, false if the requested count exceeds available data.
     */
    bool Skip(size_t count) {
        if (count > size_) {
            return false;
        }
        head_ = (head_ + count) % capacity_;
        size_ -= count;
        return true;
    }

    /**
     * @brief Gets the available contiguous write spans in the buffer.
     * 
     * Because the buffer wraps around, there can be up to two contiguous memory blocks available for writing.
     * 
     * @return An array containing up to two spans of writable memory. Spans with a size of 0 are empty.
     */
    std::array<std::span<uint8_t>, 2> GetWriteSpans() {
        if (size_ == capacity_) {
            return {};
        }

        if (size_ == 0) {
            return {
                std::span<uint8_t>(buffer_.data() + tail_, capacity_ - tail_),
                std::span<uint8_t>(buffer_.data(), tail_)
            };
        }

        if (tail_ >= head_) {
            return {
                std::span<uint8_t>(buffer_.data() + tail_, capacity_ - tail_),
                std::span<uint8_t>(buffer_.data(), head_)
            };
        } else {
            return {
                std::span<uint8_t>(buffer_.data() + tail_, head_ - tail_),
                std::span<uint8_t>()
            };
        }
    }

    /**
     * @brief Gets the available contiguous read spans in the buffer.
     * 
     * Because the buffer wraps around, there can be up to two contiguous memory blocks available for reading.
     * 
     * @return An array containing up to two spans of readable memory. Spans with a size of 0 are empty.
     */
    std::array<std::span<const uint8_t>, 2> GetReadSpans() const {
        if (size_ == 0) {
            return {};
        }

        if (tail_ > head_) {
            return {
                std::span<const uint8_t>(buffer_.data() + head_, tail_ - head_),
                std::span<const uint8_t>()
            };
        } else {
            return {
                std::span<const uint8_t>(buffer_.data() + head_, capacity_ - head_),
                std::span<const uint8_t>(buffer_.data(), tail_)
            };
        }
    }

    /**
     * @brief Advances the write pointer by the specified count.
     * @param count The number of bytes added to the buffer (e.g., after direct socket read).
     */
    void AdvanceWrite(size_t count) {
        if (count > GetFreeSpace()) {
            count = GetFreeSpace();
        }
        tail_ = (tail_ + count) % capacity_;
        size_ += count;
    }

    /**
     * @brief Gets the total capacity of the buffer.
     * @return The capacity in bytes.
     */
    size_t GetCapacity() const {
        return capacity_;
    }

    /**
     * @brief Gets the amount of currently stored data.
     * @return The size of readable data in bytes.
     */
    size_t GetSize() const {
        return size_;
    }

    /**
     * @brief Gets the amount of available free space.
     * @return The free space in bytes.
     */
    size_t GetFreeSpace() const {
        return capacity_ - size_;
    }

    /**
     * @brief Clears the buffer, resetting pointers to their initial state.
     */
    void Clear() {
        head_ = 0;
        tail_ = 0;
        size_ = 0;
    }

private:
    std::vector<uint8_t> buffer_;
    size_t capacity_;
    size_t head_;
    size_t tail_;
    size_t size_;
};

} // namespace EterBase
