#include "../src/Client/Network/PluggableCryptoProvider.h"
#include <iostream>
#include <vector>

void assert_eq(bool expected, bool actual, const std::string& msg) {
    if (expected != actual) {
        std::cerr << "Assertion failed: " << msg << std::endl;
        exit(1);
    }
}

void assert_eq(const std::vector<uint8_t>& expected, const std::vector<uint8_t>& actual, const std::string& msg) {
    if (expected != actual) {
        std::cerr << "Assertion failed: " << msg << std::endl;
        exit(1);
    }
}

void test_null_provider() {
    Client::Network::ServerProfile profile;
    auto provider = Client::Network::CreateProvider(Client::Network::CryptoType::None, profile);
    
    assert_eq(false, provider->IsActive(), "Null provider should be inactive");

    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    std::vector<uint8_t> expected = {1, 2, 3, 4, 5};

    provider->Encrypt(data);
    assert_eq(expected, data, "Null provider should not encrypt");

    provider->Decrypt(data);
    assert_eq(expected, data, "Null provider should not decrypt");
}

void test_aes_provider() {
    Client::Network::ServerProfile profile;
    auto provider = Client::Network::CreateProvider(Client::Network::CryptoType::AES, profile);
    
    assert_eq(true, provider->IsActive(), "AES provider should be active");

    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    std::vector<uint8_t> expected_encrypted = {1 ^ 0xAA, 2 ^ 0xAA, 3 ^ 0xAA, 4 ^ 0xAA, 5 ^ 0xAA};

    provider->Encrypt(data);
    assert_eq(expected_encrypted, data, "AES provider encryption mismatch");

    provider->Decrypt(data);
    std::vector<uint8_t> expected_decrypted = {1, 2, 3, 4, 5};
    assert_eq(expected_decrypted, data, "AES provider decryption mismatch");
}

void test_xtea_provider() {
    Client::Network::ServerProfile profile;
    auto provider = Client::Network::CreateProvider(Client::Network::CryptoType::XTEA, profile);
    
    assert_eq(true, provider->IsActive(), "XTEA provider should be active");

    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    std::vector<uint8_t> expected_encrypted = {static_cast<uint8_t>(~1), static_cast<uint8_t>(~2), static_cast<uint8_t>(~3), static_cast<uint8_t>(~4), static_cast<uint8_t>(~5)};

    provider->Encrypt(data);
    assert_eq(expected_encrypted, data, "XTEA provider encryption mismatch");

    provider->Decrypt(data);
    std::vector<uint8_t> expected_decrypted = {1, 2, 3, 4, 5};
    assert_eq(expected_decrypted, data, "XTEA provider decryption mismatch");
}

int main() {
    test_null_provider();
    test_aes_provider();
    test_xtea_provider();

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
