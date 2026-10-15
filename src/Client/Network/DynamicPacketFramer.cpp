#include "StdAfx.h"
#include "DynamicPacketFramer.h"
#include <cstring>
#include <algorithm>
#include <iostream>

namespace Client::Network {

DynamicPacketFramer::DynamicPacketFramer()
    : m_mode(FramingMode::Ymir1B)
{
}

void DynamicPacketFramer::SetMode(FramingMode mode) {
    m_mode = mode;
}

void DynamicPacketFramer::RegisterExpectedSize(uint8_t opcode, size_t expected_size) {
    m_expectedSizes[opcode] = expected_size;
}

void DynamicPacketFramer::AppendData(std::span<const uint8_t> data) {
    m_buffer.insert(m_buffer.end(), data.begin(), data.end());
}

EterBase::Result<std::span<const uint8_t>, EterBase::PacketError> DynamicPacketFramer::FrameNextPacket() {
    if (m_buffer.empty()) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    if (m_mode == FramingMode::Ymir1B) {
        uint8_t opcode = m_buffer[0];
        auto it = m_expectedSizes.find(opcode);
        if (it == m_expectedSizes.end()) {
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }

        size_t expected_size = it->second;
        if (m_buffer.size() < expected_size) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        m_currentPacket.assign(m_buffer.begin(), m_buffer.begin() + expected_size);
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + expected_size);

        return std::span<const uint8_t>(m_currentPacket);
    } else if (m_mode == FramingMode::M2Dev4B) {
        if (m_buffer.size() < 4) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        // Little-endian length assuming M2Dev4B: 2B opcode, 2B length
        // wait, let's read length. Usually header is 2B, length is 2B. 
        // uint16_t length = m_buffer[2] | (m_buffer[3] << 8);
        [[maybe_unused]] uint16_t opcode = m_buffer[0] | (static_cast<uint16_t>(m_buffer[1]) << 8);
        uint16_t length = m_buffer[2] | (static_cast<uint16_t>(m_buffer[3]) << 8);

        if (length < 4) {
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }

        if (m_buffer.size() < length) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        m_currentPacket.assign(m_buffer.begin(), m_buffer.begin() + length);
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + length);

        return std::span<const uint8_t>(m_currentPacket);
    }

    return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
}

void DynamicPacketFramer::Clear() {
    m_buffer.clear();
    m_currentPacket.clear();
}

} // namespace Client::Network
