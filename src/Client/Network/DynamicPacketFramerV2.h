#pragma once

#include <span>
#include <vector>
#include <deque>
#include <cstdint>
#include <unordered_map>
#include "../../EterBase/Result.h"

namespace Client::Network {

struct PacketConfig {
    bool isDynamic;
    size_t size; // Fixed size if isDynamic=false, otherwise the max expected size
    size_t lengthOffset; // Used only if isDynamic=true
    size_t lengthSize; // Size of the length field (e.g. 2 for uint16_t, 4 for uint32_t) - used only if isDynamic=true
};

class DynamicPacketFramerV2 {
public:
    DynamicPacketFramerV2();
    ~DynamicPacketFramerV2() = default;

    void RegisterFixedSize(uint8_t opcode, size_t size);
    void RegisterDynamicSize(uint8_t opcode, size_t lengthOffset, size_t lengthFieldSize, size_t maxSize);

    void AppendData(std::span<const uint8_t> data);

    EterBase::Result<std::span<const uint8_t>, EterBase::PacketError> FrameNextPacket();

    void Clear();

private:
    std::unordered_map<uint8_t, PacketConfig> m_configs;
    std::deque<uint8_t> m_buffer;
    std::vector<uint8_t> m_currentPacket;
};

} // namespace Client::Network
