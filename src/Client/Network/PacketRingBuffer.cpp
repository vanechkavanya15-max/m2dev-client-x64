#include "PacketRingBuffer.h"
#include "../../EterBase/ModernLogger.h"
#include <algorithm>
#include <cstring>

namespace Client::Network {

PacketRingBuffer::PacketRingBuffer(size_t capacity)
    : m_buffer(capacity > 0 ? capacity : 1, 0), m_readPos(0), m_writePos(0), m_capacity(capacity > 0 ? capacity : 1), m_isFull(false) {
    if (capacity == 0) {
        EterBase::ModernLogger::Warning("PacketRingBuffer initialized with capacity 0. Forcing capacity to 1 to prevent division by zero.");
    }
    EterBase::ModernLogger::Debug("PacketRingBuffer initialized with capacity: {}", m_capacity);
}

size_t PacketRingBuffer::GetAvailableSize() const noexcept {
    if (m_isFull) {
        return m_capacity;
    }
    if (m_writePos >= m_readPos) {
        return m_writePos - m_readPos;
    }
    return m_capacity - m_readPos + m_writePos;
}

size_t PacketRingBuffer::GetFreeSize() const noexcept {
    return m_capacity - GetAvailableSize();
}

size_t PacketRingBuffer::GetCapacity() const noexcept {
    return m_capacity;
}

[[nodiscard]] EterBase::VoidResult<EterBase::PacketError> PacketRingBuffer::Write(std::span<const uint8_t> data) {
    if (data.empty()) {
        return {};
    }

    if (GetFreeSize() < data.size()) {
        EterBase::ModernLogger::Error("PacketRingBuffer::Write - Buffer capacity exceeded! Free size: {}, Requested: {}", GetFreeSize(), data.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    size_t firstPartSize = std::min(data.size(), m_capacity - m_writePos);
    std::memcpy(m_buffer.data() + m_writePos, data.data(), firstPartSize);

    size_t secondPartSize = data.size() - firstPartSize;
    if (secondPartSize > 0) {
        std::memcpy(m_buffer.data(), data.data() + firstPartSize, secondPartSize);
    }

    m_writePos = (m_writePos + data.size()) % m_capacity;
    if (m_writePos == m_readPos) {
        m_isFull = true;
    }

    return {};
}

[[nodiscard]] EterBase::VoidResult<EterBase::PacketError> PacketRingBuffer::Read(std::span<uint8_t> outBuffer) {
    if (outBuffer.empty()) {
        return {};
    }

    auto result = Peek(outBuffer);
    if (!result) {
        return result;
    }

    return Skip(outBuffer.size());
}

[[nodiscard]] EterBase::VoidResult<EterBase::PacketError> PacketRingBuffer::Peek(std::span<uint8_t> outBuffer) const {
    if (outBuffer.empty()) {
        return {};
    }

    if (GetAvailableSize() < outBuffer.size()) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    size_t firstPartSize = std::min(outBuffer.size(), m_capacity - m_readPos);
    std::memcpy(outBuffer.data(), m_buffer.data() + m_readPos, firstPartSize);

    size_t secondPartSize = outBuffer.size() - firstPartSize;
    if (secondPartSize > 0) {
        std::memcpy(outBuffer.data() + firstPartSize, m_buffer.data(), secondPartSize);
    }

    return {};
}

[[nodiscard]] EterBase::VoidResult<EterBase::PacketError> PacketRingBuffer::PeekDynamicSize(size_t expectedSize) const {
    if (GetAvailableSize() < expectedSize) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }
    return {};
}

[[nodiscard]] EterBase::VoidResult<EterBase::PacketError> PacketRingBuffer::Skip(size_t size) {
    if (size == 0) {
        return {};
    }

    if (GetAvailableSize() < size) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    m_readPos = (m_readPos + size) % m_capacity;
    m_isFull = false;

    return {};
}

} // namespace Client::Network
