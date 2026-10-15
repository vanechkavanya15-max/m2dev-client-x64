#include "ZeroCopyPacketBuffer.h"

namespace Client::Network {

ZeroCopyPacketBuffer::ZeroCopyPacketBuffer(size_t capacity)
    : m_capacity(capacity > 0 ? capacity : DEFAULT_CAPACITY)
    , m_readPos(0)
    , m_writePos(0)
    , m_isFull(false)
{
    m_buffer.resize(m_capacity, 0);
    m_scratchBuffer.resize(SCRATCH_CAPACITY, 0);
}

ZeroCopyPacketBuffer::ZeroCopyPacketBuffer(ZeroCopyPacketBuffer&& other) noexcept
    : m_buffer(std::move(other.m_buffer))
    , m_capacity(other.m_capacity)
    , m_readPos(other.m_readPos)
    , m_writePos(other.m_writePos)
    , m_isFull(other.m_isFull)
    , m_scratchBuffer(std::move(other.m_scratchBuffer))
    , m_zeroCopyHits(other.m_zeroCopyHits)
    , m_wrapAroundFallbacks(other.m_wrapAroundFallbacks)
{
    other.m_capacity = 0;
    other.m_readPos = 0;
    other.m_writePos = 0;
    other.m_isFull = false;
    other.m_zeroCopyHits = 0;
    other.m_wrapAroundFallbacks = 0;
}

ZeroCopyPacketBuffer& ZeroCopyPacketBuffer::operator=(ZeroCopyPacketBuffer&& other) noexcept
{
    if (this != &other) {
        m_buffer = std::move(other.m_buffer);
        m_capacity = other.m_capacity;
        m_readPos = other.m_readPos;
        m_writePos = other.m_writePos;
        m_isFull = other.m_isFull;
        m_scratchBuffer = std::move(other.m_scratchBuffer);
        m_zeroCopyHits = other.m_zeroCopyHits;
        m_wrapAroundFallbacks = other.m_wrapAroundFallbacks;

        other.m_capacity = 0;
        other.m_readPos = 0;
        other.m_writePos = 0;
        other.m_isFull = false;
        other.m_zeroCopyHits = 0;
        other.m_wrapAroundFallbacks = 0;
    }
    return *this;
}

size_t ZeroCopyPacketBuffer::ReadableBytes() const noexcept
{
    if (m_isFull) {
        return m_capacity;
    }
    if (m_writePos >= m_readPos) {
        return m_writePos - m_readPos;
    }
    return m_capacity - m_readPos + m_writePos;
}

size_t ZeroCopyPacketBuffer::WritableBytes() const noexcept
{
    return m_capacity - ReadableBytes();
}

void ZeroCopyPacketBuffer::Clear() noexcept
{
    m_readPos = 0;
    m_writePos = 0;
    m_isFull = false;
}

std::span<uint8_t> ZeroCopyPacketBuffer::GetWritableSpan(size_t maxRequested) noexcept
{
    if (m_isFull || m_capacity == 0) {
        return {};
    }

    size_t contiguousFree = 0;
    if (m_writePos >= m_readPos) {
        // Mozemy pisac do konca bufora
        contiguousFree = m_capacity - m_writePos;
        // Jesli readPos == 0, nie mozemy przepelnic calego bufora bez ustawienia isFull
        if (m_readPos == 0 && contiguousFree > 0) {
            // max mozliwe to m_capacity
        }
    } else {
        // Mozemy pisac do readPos
        contiguousFree = m_readPos - m_writePos;
    }

    size_t availableToWrite = std::min(contiguousFree, maxRequested);
    if (availableToWrite == 0) {
        return {};
    }

    return std::span<uint8_t>(m_buffer.data() + m_writePos, availableToWrite);
}

void ZeroCopyPacketBuffer::CommitWrite(size_t bytesWritten) noexcept
{
    if (bytesWritten == 0 || m_capacity == 0) {
        return;
    }

    assert(bytesWritten <= WritableBytes());
    m_writePos = (m_writePos + bytesWritten) % m_capacity;

    if (m_writePos == m_readPos) {
        m_isFull = true;
    }
}

bool ZeroCopyPacketBuffer::Write(std::span<const uint8_t> data) noexcept
{
    if (data.empty()) {
        return true;
    }

    if (WritableBytes() < data.size()) {
        return false;
    }

    size_t firstPart = std::min(data.size(), m_capacity - m_writePos);
    std::memcpy(m_buffer.data() + m_writePos, data.data(), firstPart);

    size_t secondPart = data.size() - firstPart;
    if (secondPart > 0) {
        std::memcpy(m_buffer.data(), data.data() + firstPart, secondPart);
    }

    CommitWrite(data.size());
    return true;
}

std::span<const uint8_t> ZeroCopyPacketBuffer::PeekContiguous(size_t size) const noexcept
{
    if (size == 0 || ReadableBytes() < size || m_capacity == 0) {
        return {};
    }

    // 1. Sprawdzamy czy caly zadany blok lezy w ciaglym fragmencie przed koncem bufora
    if (m_readPos + size <= m_capacity) {
        m_zeroCopyHits++;
        return std::span<const uint8_t>(m_buffer.data() + m_readPos, size);
    }

    // 2. Wrap-around (pakiet przecina koniec bufora kołowego)
    m_wrapAroundFallbacks++;
    if (size > m_scratchBuffer.size()) {
        m_scratchBuffer.resize(size * 2);
    }

    size_t firstPart = m_capacity - m_readPos;
    size_t secondPart = size - firstPart;

    std::memcpy(m_scratchBuffer.data(), m_buffer.data() + m_readPos, firstPart);
    std::memcpy(m_scratchBuffer.data() + firstPart, m_buffer.data(), secondPart);

    return std::span<const uint8_t>(m_scratchBuffer.data(), size);
}

bool ZeroCopyPacketBuffer::CommitRead(size_t size) noexcept
{
    if (size == 0) {
        return true;
    }

    if (ReadableBytes() < size || m_capacity == 0) {
        return false;
    }

    m_readPos = (m_readPos + size) % m_capacity;
    m_isFull = false;
    return true;
}

bool ZeroCopyPacketBuffer::Peek(std::span<uint8_t> outDest) const noexcept
{
    if (outDest.empty()) {
        return true;
    }

    if (ReadableBytes() < outDest.size()) {
        return false;
    }

    auto span = PeekContiguous(outDest.size());
    if (span.size() < outDest.size()) {
        return false;
    }

    std::memcpy(outDest.data(), span.data(), outDest.size());
    return true;
}

bool ZeroCopyPacketBuffer::Read(std::span<uint8_t> outDest) noexcept
{
    if (!Peek(outDest)) {
        return false;
    }

    return CommitRead(outDest.size());
}

double ZeroCopyPacketBuffer::GetZeroCopyRatio() const noexcept
{
    uint64_t total = m_zeroCopyHits + m_wrapAroundFallbacks;
    if (total == 0) {
        return 100.0;
    }
    return (static_cast<double>(m_zeroCopyHits) / static_cast<double>(total)) * 100.0;
}

} // namespace Client::Network
