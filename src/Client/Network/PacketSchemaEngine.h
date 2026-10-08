#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <span>
#include <cstdint>
#include <expected>

namespace Client::Network {

template <typename T>
using PacketResult = std::expected<T, std::string>;

class PacketSchemaEngine {
public:
    struct FieldOffset {
        std::string name;
        size_t offset;
        size_t size;
    };

    void RegisterPacketSchema(uint32_t opcode, const std::vector<FieldOffset>& fields);
    PacketResult<uint32_t> GetFieldValueU32(uint32_t opcode, const std::string& fieldName, std::span<const uint8_t> payload) const;

private:
    std::unordered_map<uint32_t, std::vector<FieldOffset>> m_schemas;
};

} // namespace Client::Network
