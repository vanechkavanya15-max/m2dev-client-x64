#include "../src/Client/Network/AesCryptoProvider.h"
#include "../src/EterBase/LogModern.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <format>

using namespace Client::Network;

int main() {
    auto provider1 = CreateAesCryptoProvider();
    auto provider2 = CreateAesCryptoProvider();

    // Test 1: Basic encryption and decryption (symmetric property of CTR)
    std::vector<uint8_t> plainText = { 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
    std::vector<uint8_t> buffer = plainText;

    EterBase::ModernLogger::Info("Testing basic AES-CTR encryption...");
    auto encRes = provider1->ProcessStream(buffer.data(), buffer.size());
    if (!encRes.has_value()) {
        EterBase::ModernLogger::Error("Encryption failed: {}", encRes.error());
        return 1;
    }

    if (std::memcmp(buffer.data(), plainText.data(), buffer.size()) == 0) {
        EterBase::ModernLogger::Error("Encryption did not change the data");
        return 1;
    }

    EterBase::ModernLogger::Info("Testing basic AES-CTR decryption...");
    auto decRes = provider2->ProcessStream(buffer.data(), buffer.size());
    if (!decRes.has_value()) {
        EterBase::ModernLogger::Error("Decryption failed: {}", decRes.error());
        return 1;
    }

    if (std::memcmp(buffer.data(), plainText.data(), buffer.size()) != 0) {
        EterBase::ModernLogger::Error("Decrypted data does not match original plain text");
        return 1;
    }
    
    // Test 2: Variable length stream processing without breaking header offsets
    EterBase::ModernLogger::Info("Testing variable length AES-CTR stream encryption/decryption...");
    
    auto provider3 = CreateAesCryptoProvider();
    auto provider4 = CreateAesCryptoProvider();
    
    std::vector<uint8_t> part1 = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    std::vector<uint8_t> part2 = { 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15 };
    
    std::vector<uint8_t> buf1 = part1;
    std::vector<uint8_t> buf2 = part2;

    provider3->ProcessStream(buf1.data(), buf1.size());
    provider3->ProcessStream(buf2.data(), buf2.size());
    
    provider4->ProcessStream(buf1.data(), buf1.size());
    provider4->ProcessStream(buf2.data(), buf2.size());
    
    if (std::memcmp(buf1.data(), part1.data(), buf1.size()) != 0 || 
        std::memcmp(buf2.data(), part2.data(), buf2.size()) != 0) {
        EterBase::ModernLogger::Error("Variable length processing failed to correctly restore data");
        return 1;
    }

    EterBase::ModernLogger::Info("All tests passed successfully.");
    return 0;
}
