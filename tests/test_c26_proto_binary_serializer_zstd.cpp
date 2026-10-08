#include "../src/Client/Data/ProtoBinarySerializerZstd.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <cassert>

int main() {
    std::vector<ItemProtoRecord> originalRecords(1000);
    
    // Fill with dummy data
    for (size_t i = 0; i < 1000; ++i) {
        originalRecords[i].vnum = 10000 + i;
        std::snprintf(originalRecords[i].name, sizeof(originalRecords[i].name), "Item_%zu", i);
        originalRecords[i].type = static_cast<uint8_t>(i % 10);
        originalRecords[i].subType = static_cast<uint8_t>(i % 5);
        originalRecords[i].flags = 0;
        originalRecords[i].buyPrice = static_cast<uint32_t>(i * 100);
        originalRecords[i].sellPrice = static_cast<uint32_t>(i * 50);
    }

    // Test Serialization
    std::vector<uint8_t> serializedBuffer;
    try {
        serializedBuffer = ProtoBinarySerializerZstd::Serialize(originalRecords);
        std::cout << "Serialized 1000 records to " << serializedBuffer.size() << " bytes." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Serialization failed: " << e.what() << std::endl;
        return 1;
    }

    // Test Deserialization
    auto deserializedResult = ProtoBinarySerializerZstd::Deserialize(serializedBuffer);
    if (!deserializedResult) {
        std::cerr << "Deserialization failed with error code: " << static_cast<int>(deserializedResult.error()) << std::endl;
        return 1;
    }

    const auto& deserializedRecords = deserializedResult.value();
    
    // Validate Round Trip
    assert(deserializedRecords.size() == 1000 && "Record count mismatch");
    
    for (size_t i = 0; i < 1000; ++i) {
        assert(originalRecords[i].vnum == deserializedRecords[i].vnum);
        assert(std::strcmp(originalRecords[i].name, deserializedRecords[i].name) == 0);
        assert(originalRecords[i].type == deserializedRecords[i].type);
        assert(originalRecords[i].subType == deserializedRecords[i].subType);
        assert(originalRecords[i].flags == deserializedRecords[i].flags);
        assert(originalRecords[i].buyPrice == deserializedRecords[i].buyPrice);
        assert(originalRecords[i].sellPrice == deserializedRecords[i].sellPrice);
    }

    std::cout << "Round-trip compression/decompression validated successfully." << std::endl;

    return 0;
}
