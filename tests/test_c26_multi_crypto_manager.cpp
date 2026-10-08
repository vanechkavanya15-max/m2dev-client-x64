#undef _WIN32
#include "../src/EterBase/LogModern.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <cstring>

namespace Client::Network {
    class AesCryptoProvider;
}

#include "../src/Client/Network/ServerProfile.h"
#include "../src/Client/Network/MultiCryptoManager.h"

namespace Client::Network {
    void ServerProfile::SetCryptoType(CryptoType type) { m_cryptoType = type; }
    CryptoType ServerProfile::GetCryptoType() const { return m_cryptoType; }
}

namespace {
    class MockAesCryptoProvider final : public Client::Network::ICryptoProvider {
    public:
        EterBase::VoidResult<std::string_view> ProcessStream(uint8_t* data, size_t length) override {
            if (!data && length > 0) return EterBase::MakeError("Invalid input");
            for (size_t i = 0; i < length; ++i) data[i] ^= 0xAA;
            return {};
        }
    };
}

namespace Client::Network {
    std::unique_ptr<ICryptoProvider> CreateAesCryptoProvider() {
        return std::make_unique<MockAesCryptoProvider>();
    }
}

#include "../src/Client/Network/MultiCryptoManager.cpp"

using namespace Client::Network;

int main() {
    EterBase::ModernLogger::Info("Testing MultiCryptoManager...");

    ServerProfile plainProfile;
    plainProfile.SetCryptoType(CryptoType::Plain);
    
    MultiCryptoManager plainManager(plainProfile);
    if (plainManager.IsEncryptionActive()) {
        EterBase::ModernLogger::Error("Encryption should be inactive by default");
        return 1;
    }

    std::vector<uint8_t> plainData = { 1, 2, 3, 4, 5 };
    std::vector<uint8_t> plainDataCopy = plainData;
    
    auto res = plainManager.ProcessStream(plainData.data(), plainData.size());
    if (!res.has_value()) {
        EterBase::ModernLogger::Error("ProcessStream failed for plain inactive");
        return 1;
    }
    
    plainManager.ActivateEncryption();
    if (!plainManager.IsEncryptionActive()) {
        EterBase::ModernLogger::Error("Encryption should be active");
        return 1;
    }
    
    res = plainManager.ProcessStream(plainData.data(), plainData.size());
    if (std::memcmp(plainData.data(), plainDataCopy.data(), plainData.size()) != 0) {
        EterBase::ModernLogger::Error("Plain crypto should not modify data");
        return 1;
    }

    ServerProfile teaProfile;
    teaProfile.SetCryptoType(CryptoType::TEA);
    MultiCryptoManager teaManager(teaProfile);
    
    std::vector<uint8_t> teaData = { 1, 2, 3, 4, 5 };
    std::vector<uint8_t> testKey = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };
    
    auto teaRes = teaManager.ProcessStream(teaData.data(), teaData.size());
    if (std::memcmp(teaData.data(), plainDataCopy.data(), teaData.size()) != 0) {
        EterBase::ModernLogger::Error("TEA crypto modified data when inactive");
        return 1;
    }

    teaManager.ActivateEncryption();
    teaRes = teaManager.ProcessStream(teaData.data(), teaData.size());
    if (teaRes.has_value()) {
        EterBase::ModernLogger::Error("TEA crypto should fail if key is not set");
        return 1;
    }

    if (!teaManager.SetSessionKey(testKey).has_value()) {
        EterBase::ModernLogger::Error("Failed to set session key for TEA");
        return 1;
    }
    teaRes = teaManager.ProcessStream(teaData.data(), teaData.size());
    
    if (std::memcmp(teaData.data(), plainDataCopy.data(), teaData.size()) == 0) {
        EterBase::ModernLogger::Error("TEA crypto did not modify data when active");
        return 1;
    }

    MultiCryptoManager teaManagerDec(teaProfile);
    if (!teaManagerDec.SetSessionKey(testKey).has_value()) {
        EterBase::ModernLogger::Error("Failed to set session key for TEA decrypt");
        return 1;
    }
    teaManagerDec.ActivateEncryption();
    teaRes = teaManagerDec.ProcessStream(teaData.data(), teaData.size());
    
    if (std::memcmp(teaData.data(), plainDataCopy.data(), teaData.size()) != 0) {
        EterBase::ModernLogger::Error("TEA crypto did not symmetrically decrypt data correctly");
        return 1;
    }

    ServerProfile aesProfile;
    aesProfile.SetCryptoType(CryptoType::AES);
    MultiCryptoManager aesManager(aesProfile);
    
    std::vector<uint8_t> aesData = { 1, 2, 3, 4, 5 };
    auto aesRes = aesManager.ProcessStream(aesData.data(), aesData.size());
    if (std::memcmp(aesData.data(), plainDataCopy.data(), aesData.size()) != 0) {
        EterBase::ModernLogger::Error("AES crypto modified data when inactive");
        return 1;
    }

    aesManager.ActivateEncryption();
    aesRes = aesManager.ProcessStream(aesData.data(), aesData.size());
    if (std::memcmp(aesData.data(), plainDataCopy.data(), aesData.size()) == 0) {
        EterBase::ModernLogger::Error("AES crypto did not modify data when active");
        return 1;
    }
    
    std::vector<uint8_t> expectedAesData = { 1 ^ 0xAA, 2 ^ 0xAA, 3 ^ 0xAA, 4 ^ 0xAA, 5 ^ 0xAA };
    if (std::memcmp(aesData.data(), expectedAesData.data(), aesData.size()) != 0) {
        EterBase::ModernLogger::Error("AES crypto did not modify data correctly");
        return 1;
    }
    
    MultiCryptoManager aesManagerDec(aesProfile);
    aesManagerDec.ActivateEncryption();
    aesRes = aesManagerDec.ProcessStream(aesData.data(), aesData.size());
    if (std::memcmp(aesData.data(), plainDataCopy.data(), aesData.size()) != 0) {
        EterBase::ModernLogger::Error("AES crypto did not symmetrically decrypt data correctly");
        return 1;
    }
    
    auto errRes = teaManager.ProcessStream(nullptr, 10);
    if (errRes.has_value()) {
        EterBase::ModernLogger::Error("ProcessStream should fail with null pointer and length > 0");
        return 1;
    }
    
    errRes = teaManager.ProcessStream(nullptr, 0);
    if (!errRes.has_value()) {
        EterBase::ModernLogger::Error("ProcessStream should succeed with null pointer and length 0");
        return 1;
    }
    
    EterBase::ModernLogger::Info("All tests passed successfully.");
    return 0;
}
