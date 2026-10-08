#pragma once

#include <cstdint>
#include <span>
#include <array>
#include <expected>

namespace Client::Network {

enum class CryptoError {
    InvalidKeySize,
    InvalidBlockSize,
    NotInitialized
};

class ICryptoProvider {
public:
    virtual ~ICryptoProvider() = default;

    virtual std::expected<void, CryptoError> Initialize(std::span<const uint8_t> key) = 0;
    virtual std::expected<void, CryptoError> EncryptBlock(std::span<uint8_t> block) = 0;
    virtual std::expected<void, CryptoError> DecryptBlock(std::span<uint8_t> block) = 0;
};

class TeaCryptoProvider : public ICryptoProvider {
public:
    static constexpr size_t KEY_SIZE = 16;
    static constexpr size_t BLOCK_SIZE = 8;
    static constexpr uint32_t DELTA = 0x9E3779B9;
    static constexpr uint32_t NUM_ROUNDS = 32;

    TeaCryptoProvider() = default;
    ~TeaCryptoProvider() override = default;

    std::expected<void, CryptoError> Initialize(std::span<const uint8_t> key) override;
    std::expected<void, CryptoError> EncryptBlock(std::span<uint8_t> block) override;
    std::expected<void, CryptoError> DecryptBlock(std::span<uint8_t> block) override;

private:
    std::array<uint32_t, 4> m_key{};
    bool m_initialized = false;
};

} // namespace Client::Network
