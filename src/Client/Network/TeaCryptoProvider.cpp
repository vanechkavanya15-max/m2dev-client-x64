#include "TeaCryptoProvider.h"

#include <cstring>
#include <bit>

namespace Client::Network {

std::expected<void, CryptoError> TeaCryptoProvider::Initialize(std::span<const uint8_t> key) {
    if (key.size() != KEY_SIZE) {
        return std::unexpected(CryptoError::InvalidKeySize);
    }

    std::memcpy(m_key.data(), key.data(), KEY_SIZE);
    m_initialized = true;

    return {};
}

std::expected<void, CryptoError> TeaCryptoProvider::EncryptBlock(std::span<uint8_t> block) {
    if (!m_initialized) {
        return std::unexpected(CryptoError::NotInitialized);
    }

    if (block.size() != BLOCK_SIZE) {
        return std::unexpected(CryptoError::InvalidBlockSize);
    }

    uint32_t v0;
    uint32_t v1;
    std::memcpy(&v0, block.data(), sizeof(uint32_t));
    std::memcpy(&v1, block.data() + sizeof(uint32_t), sizeof(uint32_t));

    uint32_t sum = 0;
    for (uint32_t i = 0; i < NUM_ROUNDS; ++i) {
        sum += DELTA;
        v0 += ((v1 << 4) + m_key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + m_key[1]);
        v1 += ((v0 << 4) + m_key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + m_key[3]);
    }

    std::memcpy(block.data(), &v0, sizeof(uint32_t));
    std::memcpy(block.data() + sizeof(uint32_t), &v1, sizeof(uint32_t));

    return {};
}

std::expected<void, CryptoError> TeaCryptoProvider::DecryptBlock(std::span<uint8_t> block) {
    if (!m_initialized) {
        return std::unexpected(CryptoError::NotInitialized);
    }

    if (block.size() != BLOCK_SIZE) {
        return std::unexpected(CryptoError::InvalidBlockSize);
    }

    uint32_t v0;
    uint32_t v1;
    std::memcpy(&v0, block.data(), sizeof(uint32_t));
    std::memcpy(&v1, block.data() + sizeof(uint32_t), sizeof(uint32_t));

    uint32_t sum = DELTA * NUM_ROUNDS;
    for (uint32_t i = 0; i < NUM_ROUNDS; ++i) {
        v1 -= ((v0 << 4) + m_key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + m_key[3]);
        v0 -= ((v1 << 4) + m_key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + m_key[1]);
        sum -= DELTA;
    }

    std::memcpy(block.data(), &v0, sizeof(uint32_t));
    std::memcpy(block.data() + sizeof(uint32_t), &v1, sizeof(uint32_t));

    return {};
}

} // namespace Client::Network
