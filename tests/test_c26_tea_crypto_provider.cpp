#include <cassert>
#include <iostream>
#include <vector>
#include <cstring>
#include "../src/Client/Network/TeaCryptoProvider.h"

using namespace Client::Network;

void TestInitialization() {
    TeaCryptoProvider provider;
    
    // Test invalid key size
    std::vector<uint8_t> invalid_key(15, 0x11);
    auto init_result = provider.Initialize(invalid_key);
    assert(!init_result.has_value());
    assert(init_result.error() == CryptoError::InvalidKeySize);

    // Test valid key size
    std::vector<uint8_t> valid_key(16, 0x11);
    init_result = provider.Initialize(valid_key);
    assert(init_result.has_value());
    std::cout << "TestInitialization passed." << std::endl;
}

void TestEncryptionWithoutInitialization() {
    TeaCryptoProvider provider;
    std::vector<uint8_t> block(8, 0x00);
    
    auto enc_result = provider.EncryptBlock(block);
    assert(!enc_result.has_value());
    assert(enc_result.error() == CryptoError::NotInitialized);
    
    auto dec_result = provider.DecryptBlock(block);
    assert(!dec_result.has_value());
    assert(dec_result.error() == CryptoError::NotInitialized);
    
    std::cout << "TestEncryptionWithoutInitialization passed." << std::endl;
}

void TestInvalidBlockSize() {
    TeaCryptoProvider provider;
    std::vector<uint8_t> key(16, 0xAB);
    provider.Initialize(key);
    
    std::vector<uint8_t> invalid_block(7, 0x00);
    auto enc_result = provider.EncryptBlock(invalid_block);
    assert(!enc_result.has_value());
    assert(enc_result.error() == CryptoError::InvalidBlockSize);
    
    auto dec_result = provider.DecryptBlock(invalid_block);
    assert(!dec_result.has_value());
    assert(dec_result.error() == CryptoError::InvalidBlockSize);
    
    std::cout << "TestInvalidBlockSize passed." << std::endl;
}

void TestEncryptionDecryption() {
    TeaCryptoProvider provider;
    std::vector<uint8_t> key = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };
    
    auto init_result = provider.Initialize(key);
    assert(init_result.has_value());
    
    std::vector<uint8_t> original_block = {
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
    };
    std::vector<uint8_t> block = original_block;
    
    auto enc_result = provider.EncryptBlock(block);
    assert(enc_result.has_value());
    
    // Check if block was changed
    assert(block != original_block);
    
    auto dec_result = provider.DecryptBlock(block);
    assert(dec_result.has_value());
    
    // Check if decrypted block matches original block
    assert(block == original_block);
    
    std::cout << "TestEncryptionDecryption passed." << std::endl;
}

int main() {
    std::cout << "Running TeaCryptoProvider tests..." << std::endl;
    
    TestInitialization();
    TestEncryptionWithoutInitialization();
    TestInvalidBlockSize();
    TestEncryptionDecryption();
    
    std::cout << "All TeaCryptoProvider tests passed successfully." << std::endl;
    return 0;
}
