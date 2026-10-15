#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <expected>

enum class ProtoError : uint8_t {
    None = 0,
    CompressionFailed,
    DecompressionFailed,
    InvalidHeader,
    MagicMismatch,
    VersionMismatch,
    CRC32Mismatch
};

struct ProtoModernHeader {
    uint32_t magic;
    uint32_t formatVersion;
    uint32_t schemaHash;
    uint32_t recordCount;
    uint32_t uncompressedSize;
    uint32_t compressedSize;
    uint32_t checksumCRC32;
};

struct ItemProtoRecord {
    uint32_t vnum;
    char name[65];
    uint8_t type;
    uint8_t subType;
    uint32_t flags;
    uint32_t buyPrice;
    uint32_t sellPrice;
};

class ProtoBinarySerializerZstd {
public:
    static std::vector<uint8_t> Serialize(std::span<const ItemProtoRecord> records);
    static std::expected<std::vector<ItemProtoRecord>, ProtoError> Deserialize(std::span<const uint8_t> buffer);
};
