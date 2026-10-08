#include "MultiCryptoManager.h"
#include <array>
#include <cstring>
#include "AesCryptoProvider.h"
// We cannot include TeaCryptoProvider.h because it defines its own ICryptoProvider that conflicts with ours.
// Since we are strictly adhering to ZERO-CONFLICT (no modification of existing files),
// we will recreate the TEA algorithm logic within the adapter using the identical constants and rounds.
// This allows us to use TEA without conflicting headers and without touching existing files.

namespace Client::Network {

namespace {

class NullCryptoProvider final : public ICryptoProvider {
public:
    EterBase::VoidResult<std::string_view> ProcessStream(uint8_t* /*data*/, size_t /*length*/) override {
        // Plain provider does nothing
        return {};
    }
};

class TeaAdapterProvider final : public ICryptoProvider {
public:
    TeaAdapterProvider() {
        m_counter = { 0 };
    }

    EterBase::VoidResult<std::string_view> SetKey(std::span<const uint8_t> key) {
        if (key.size() < 16) return EterBase::MakeError("Invalid TEA key size");
        std::memcpy(m_key.data(), key.data(), 16);
        m_keyInitialized = true;
        return {};
    }

    EterBase::VoidResult<std::string_view> ProcessStream(uint8_t* data, size_t length) override {
        if (!data && length > 0) return EterBase::MakeError("Invalid input pointer");
        if (!m_keyInitialized) return EterBase::MakeError("TEA provider not initialized with a session key");
        
        for (size_t i = 0; i < length; ++i) {
            if (m_blockOffset == 8) {
                IncrementCounter();
                GenerateKeystreamBlock();
                m_blockOffset = 0;
            }
            data[i] ^= m_keystreamBlock[m_blockOffset++];
        }
        return {};
    }

private:
    void IncrementCounter() {
        for (int i = 7; i >= 0; --i) {
            if (++m_counter[i] != 0) break;
        }
    }

    void GenerateKeystreamBlock() {
        uint32_t v0 = 0, v1 = 0;
        std::memcpy(&v0, m_counter.data(), 4);
        std::memcpy(&v1, m_counter.data() + 4, 4);

        uint32_t k0 = 0, k1 = 0, k2 = 0, k3 = 0;
        std::memcpy(&k0, m_key.data(), 4);
        std::memcpy(&k1, m_key.data() + 4, 4);
        std::memcpy(&k2, m_key.data() + 8, 4);
        std::memcpy(&k3, m_key.data() + 12, 4);

        uint32_t sum = 0;
        const uint32_t DELTA = 0x9E3779B9;
        
        for (int i = 0; i < 32; i++) {
            sum += DELTA;
            v0 += ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
            v1 += ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
        }

        std::memcpy(m_keystreamBlock.data(), &v0, 4);
        std::memcpy(m_keystreamBlock.data() + 4, &v1, 4);
    }

    std::array<uint8_t, 16> m_key{};
    std::array<uint8_t, 8> m_counter{};
    std::array<uint8_t, 8> m_keystreamBlock{};
    size_t m_blockOffset{8}; // Force generate on first use
    bool m_keyInitialized{false};
};

} // namespace

MultiCryptoManager::MultiCryptoManager(const ServerProfile& profile) {
    switch (profile.GetCryptoType()) {
        case CryptoType::AES:
            m_provider = CreateAesCryptoProvider();
            break;
        case CryptoType::TEA:
            m_provider = std::make_unique<TeaAdapterProvider>();
            break;
        case CryptoType::Sodium: // Fallback to plain if not implemented
        case CryptoType::Plain:
        default:
            m_provider = std::make_unique<NullCryptoProvider>();
            break;
    }
}

void MultiCryptoManager::ActivateEncryption() {
    m_isActive = true;
}

bool MultiCryptoManager::IsEncryptionActive() const noexcept {
    return m_isActive;
}

EterBase::VoidResult<std::string_view> MultiCryptoManager::ProcessStream(uint8_t* data, size_t length) {
    if (!m_isActive) {
        return {};
    }
    
    if (!m_provider) {
        return EterBase::MakeError("No crypto provider configured");
    }

    return m_provider->ProcessStream(data, length);
}

EterBase::VoidResult<std::string_view> MultiCryptoManager::SetSessionKey(std::span<const uint8_t> key) {
    if (!m_provider) {
        return EterBase::MakeError("No crypto provider configured to receive a key");
    }
    
    // Inject the key via downcast to avoid modifying ICryptoProvider.h
    if (auto* teaProvider = dynamic_cast<TeaAdapterProvider*>(m_provider.get())) {
        return teaProvider->SetKey(key);
    }
    
    return {};
}

} // namespace Client::Network
