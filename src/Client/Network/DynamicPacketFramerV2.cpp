#include "StdAfx.h"
#include "DynamicPacketFramerV2.h"
#include <cstring>
#include <algorithm>

namespace Client::Network {

DynamicPacketFramerV2::DynamicPacketFramerV2() = default;

void DynamicPacketFramerV2::RegisterFixedSize(uint8_t opcode, size_t size) {
    m_configs[opcode] = PacketConfig{false, size, 0, 0};
}

void DynamicPacketFramerV2::RegisterDynamicSize(uint8_t opcode, size_t lengthOffset, size_t lengthFieldSize, size_t maxSize) {
    m_configs[opcode] = PacketConfig{true, maxSize, lengthOffset, lengthFieldSize};
}

void DynamicPacketFramerV2::AppendData(std::span<const uint8_t> data) {
    m_buffer.insert(m_buffer.end(), data.begin(), data.end());
}

EterBase::Result<std::span<const uint8_t>, EterBase::PacketError> DynamicPacketFramerV2::FrameNextPacket() {
    if (m_buffer.empty()) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    uint8_t opcode = m_buffer.front();
    auto it = m_configs.find(opcode);
    if (it == m_configs.end()) {
        return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
    }

    const auto& config = it->second;

    size_t expectedSize = 0;

    if (!config.isDynamic) {
        expectedSize = config.size;
    } else {
        // Dynamic length
        size_t minSizeToReadLength = config.lengthOffset + config.lengthSize;
        if (m_buffer.size() < minSizeToReadLength) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        size_t length = 0;
        if (config.lengthSize == 1) {
            length = m_buffer[config.lengthOffset];
        } else if (config.lengthSize == 2) {
            length = m_buffer[config.lengthOffset] | (static_cast<size_t>(m_buffer[config.lengthOffset + 1]) << 8);
        } else if (config.lengthSize == 4) {
            length = m_buffer[config.lengthOffset] | 
                     (static_cast<size_t>(m_buffer[config.lengthOffset + 1]) << 8) |
                     (static_cast<size_t>(m_buffer[config.lengthOffset + 2]) << 16) |
                     (static_cast<size_t>(m_buffer[config.lengthOffset + 3]) << 24);
        } else {
            // Unsupported length field size
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }

        if (length < minSizeToReadLength || length > config.size) {
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        expectedSize = length;
    }

    if (m_buffer.size() < expectedSize) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    m_currentPacket.assign(m_buffer.begin(), m_buffer.begin() + expectedSize);
    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + expectedSize);

    return std::span<const uint8_t>(m_currentPacket);
}

void DynamicPacketFramerV2::Clear() {
    m_buffer.clear();
    m_currentPacket.clear();
}

} // namespace Client::Network
