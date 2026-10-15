#pragma once

#include "ICryptoProvider.h"
#include <array>

namespace Client::Network {

/**
 * @brief AES-CTR crypto provider implementation.
 * Ensures stream encryption/decryption without damaging headers, maintaining
 * the counter block and internal offset across multiple stream inputs.
 */
class AesCryptoProvider final : public ICryptoProvider {
public:
    AesCryptoProvider();
    ~AesCryptoProvider() override = default;

    EterBase::VoidResult<std::string_view> ProcessStream(uint8_t* data, size_t length) override;

private:
    void IncrementCounter();
    void GenerateKeystreamBlock();

    std::array<uint8_t, 32> m_key{};
    std::array<uint8_t, 240> m_roundKeys{}; // Expanded key for AES-256
    std::array<uint8_t, 16> m_counter{};
    std::array<uint8_t, 16> m_keystreamBlock{};
    size_t m_blockOffset = 0;
};

} // namespace Client::Network
