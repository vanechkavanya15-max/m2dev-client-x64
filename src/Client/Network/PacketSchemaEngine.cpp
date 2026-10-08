#include "PacketSchemaEngine.h"
#include <cstring>
#include <algorithm>

namespace Client::Network {

void PacketSchemaEngine::RegisterPacketSchema(uint32_t opcode, const std::vector<FieldOffset>& fields) {
    m_schemas[opcode] = fields;
}

PacketResult<uint32_t> PacketSchemaEngine::GetFieldValueU32(uint32_t opcode, const std::string& fieldName, std::span<const uint8_t> payload) const {
    auto it = m_schemas.find(opcode);
    if (it == m_schemas.end()) {
        return std::unexpected("Schema not found for opcode");
    }

    const auto& fields = it->second;
    auto fieldIt = std::find_if(fields.begin(), fields.end(), [&fieldName](const FieldOffset& field) {
        return field.name == fieldName;
    });

    if (fieldIt == fields.end()) {
        return std::unexpected("Field not found in schema");
    }

    if (fieldIt->size > sizeof(uint32_t)) {
        return std::unexpected("Field size exceeds u32");
    }

    if (fieldIt->offset + fieldIt->size > payload.size()) {
        return std::unexpected("Buffer underflow");
    }

    uint32_t value = 0;
    std::memcpy(&value, payload.data() + fieldIt->offset, fieldIt->size);
    return value;
}

} // namespace Client::Network
