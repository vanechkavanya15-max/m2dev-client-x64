#pragma once

#include "NetCryptoProvider.h"
#include "EterBase/SecureCipher.h"
#include <mutex>

/**
 * @brief SodiumCryptoProvider implementujacy INetCryptoProvider wrapujacy istniejacy SecureCipher (libsodium / XChaCha20-Poly1305).
 */
class SodiumCryptoProvider final : public INetCryptoProvider
{
public:
    SodiumCryptoProvider();
    ~SodiumCryptoProvider() override;

    // INetCryptoProvider interface
    void EncryptInPlace(void* buffer, size_t len) override;
    void DecryptInPlace(void* buffer, size_t len) override;
    bool IsActivated() const noexcept override;
    void SetActivated(bool value) noexcept override;
    void CleanUp() noexcept override;
    std::string_view GetProviderName() const noexcept override;

    // Dostep do opakowanego SecureCipher dla operacji handshaku (wymiana kluczy X25519, token sesyjny)
    SecureCipher& GetCipher() noexcept { return m_cipher; }
    const SecureCipher& GetCipher() const noexcept { return m_cipher; }

private:
    SecureCipher m_cipher;
    mutable std::mutex m_txMutex;
    mutable std::mutex m_rxMutex;
};
