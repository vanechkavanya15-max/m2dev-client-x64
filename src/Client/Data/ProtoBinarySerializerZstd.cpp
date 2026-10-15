#include "ProtoBinarySerializerZstd.h"
#include <zstd.h>
#include <cstring>
#include <stdexcept>

// Simple CRC32 for testing/placeholder
static uint32_t CalculateCRC32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (size_t j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

std::vector<uint8_t> ProtoBinarySerializerZstd::Serialize(std::span<const ItemProtoRecord> records) {
    ProtoModernHeader header{};
    header.magic = 0x5A505254; // "ZPRT"
    header.formatVersion = 1;
    header.schemaHash = 0; // Not strictly validated for this test
    header.recordCount = static_cast<uint32_t>(records.size());
    header.uncompressedSize = static_cast<uint32_t>(records.size() * sizeof(ItemProtoRecord));

    size_t compressBound = ZSTD_compressBound(header.uncompressedSize);
    std::vector<uint8_t> compressedBuffer(compressBound);

    size_t cSize = ZSTD_compress(compressedBuffer.data(), compressBound,
                                 records.data(), header.uncompressedSize,
                                 1); // Compression level 1 for speed
    
    if (ZSTD_isError(cSize)) {
        throw std::runtime_error("ZSTD compression failed");
    }

    compressedBuffer.resize(cSize);
    header.compressedSize = static_cast<uint32_t>(cSize);
    header.checksumCRC32 = CalculateCRC32(compressedBuffer.data(), cSize);

    std::vector<uint8_t> finalBuffer;
    finalBuffer.reserve(sizeof(ProtoModernHeader) + cSize);

    uint8_t* headerPtr = reinterpret_cast<uint8_t*>(&header);
    finalBuffer.insert(finalBuffer.end(), headerPtr, headerPtr + sizeof(ProtoModernHeader));
    finalBuffer.insert(finalBuffer.end(), compressedBuffer.begin(), compressedBuffer.end());

    return finalBuffer;
}

std::expected<std::vector<ItemProtoRecord>, ProtoError> ProtoBinarySerializerZstd::Deserialize(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(ProtoModernHeader)) {
        return std::unexpected(ProtoError::InvalidHeader);
    }

    ProtoModernHeader header;
    std::memcpy(&header, buffer.data(), sizeof(ProtoModernHeader));

    if (header.magic != 0x5A505254) {
        return std::unexpected(ProtoError::MagicMismatch);
    }
    if (header.formatVersion != 1) {
        return std::unexpected(ProtoError::VersionMismatch);
    }
    
    std::span<const uint8_t> compressedData = buffer.subspan(sizeof(ProtoModernHeader));
    if (compressedData.size() != header.compressedSize) {
        return std::unexpected(ProtoError::InvalidHeader);
    }

    uint32_t computedCRC = CalculateCRC32(compressedData.data(), compressedData.size());
    if (computedCRC != header.checksumCRC32) {
        return std::unexpected(ProtoError::CRC32Mismatch);
    }

    if (header.uncompressedSize != header.recordCount * sizeof(ItemProtoRecord)) {
        return std::unexpected(ProtoError::InvalidHeader);
    }

    std::vector<ItemProtoRecord> decompressedRecords(header.recordCount);
    size_t dSize = ZSTD_decompress(decompressedRecords.data(), decompressedRecords.size() * sizeof(ItemProtoRecord),
                                   compressedData.data(), compressedData.size());

    if (ZSTD_isError(dSize) || dSize != header.uncompressedSize) {
        return std::unexpected(ProtoError::DecompressionFailed);
    }

    return decompressedRecords;
}
