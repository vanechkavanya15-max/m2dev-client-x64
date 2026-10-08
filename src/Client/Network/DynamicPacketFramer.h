#pragma once

#include <span>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include "../../EterBase/Result.h"

namespace Client::Network {

enum class FramingMode {
    Ymir1B,
    M2Dev4B
};

class DynamicPacketFramer {
public:
    DynamicPacketFramer();
    ~DynamicPacketFramer() = default;

    void SetMode(FramingMode mode);
    void RegisterExpectedSize(uint8_t opcode, size_t expected_size);
    
    void AppendData(std::span<const uint8_t> data);
    
    EterBase::Result<std::span<const uint8_t>, EterBase::PacketError> FrameNextPacket();

    void Clear();

private:
    FramingMode m_mode;
    std::unordered_map<uint8_t, size_t> m_expectedSizes;
    std::vector<uint8_t> m_buffer;
    std::vector<uint8_t> m_currentPacket;
};

} // namespace Client::Network
