#pragma once

#include <cstdint>
#include <span>
#include <cstring>
#include <cstddef>

/**
 * @brief Modern C++20 Tiny Encryption Algorithm (TEA) implementation.
 * 
 * Provides static utility functions to encrypt and decrypt byte spans in-place 
 * without relying on raw pointers in the API. It operates directly on 128-bit keys
 * and memory blocks in a memory-safe manner.
 */
class TeaModern {
public:
    /** @brief Expected key length in bytes (128-bit). */
    static constexpr std::size_t KeySize = 16;
    
    /** @brief TEA block size in bytes (64-bit). */
    static constexpr std::size_t BlockSize = 8;

    /**
     * @brief Encrypts the provided data buffer in-place using TEA.
     * 
     * @param buffer Mutable byte buffer to encrypt. Length must be a multiple of 8 bytes.
     * @param key A 128-bit key (exactly 16 bytes).
     * @return true if encryption succeeded, false on size mismatch.
     */
    static bool Encrypt(std::span<uint8_t> buffer, std::span<const uint8_t> key) noexcept {
        if (key.size() != KeySize || (buffer.size() % BlockSize) != 0) {
            return false;
        }

        uint32_t k[4] = {0};
        std::memcpy(k, key.data(), KeySize);

        for (std::size_t offset = 0; offset < buffer.size(); offset += BlockSize) {
            uint32_t v[2] = {0};
            std::memcpy(v, buffer.subspan(offset, BlockSize).data(), BlockSize);
            
            uint32_t v0 = v[0];
            uint32_t v1 = v[1];
            uint32_t sum = 0;
            constexpr uint32_t delta = 0x9e3779b9;
            
            for (uint32_t cycle = 0; cycle < 32; ++cycle) {
                sum += delta;
                v0 += ((v1 << 4) + k[0]) ^ (v1 + sum) ^ ((v1 >> 5) + k[1]);
                v1 += ((v0 << 4) + k[2]) ^ (v0 + sum) ^ ((v0 >> 5) + k[3]);
            }
            
            v[0] = v0;
            v[1] = v1;
            std::memcpy(buffer.subspan(offset, BlockSize).data(), v, BlockSize);
        }

        return true;
    }

    /**
     * @brief Decrypts the provided data buffer in-place using TEA.
     * 
     * @param buffer Mutable byte buffer to decrypt. Length must be a multiple of 8 bytes.
     * @param key A 128-bit key (exactly 16 bytes).
     * @return true if decryption succeeded, false on size mismatch.
     */
    static bool Decrypt(std::span<uint8_t> buffer, std::span<const uint8_t> key) noexcept {
        if (key.size() != KeySize || (buffer.size() % BlockSize) != 0) {
            return false;
        }

        uint32_t k[4] = {0};
        std::memcpy(k, key.data(), KeySize);

        for (std::size_t offset = 0; offset < buffer.size(); offset += BlockSize) {
            uint32_t v[2] = {0};
            std::memcpy(v, buffer.subspan(offset, BlockSize).data(), BlockSize);
            
            uint32_t v0 = v[0];
            uint32_t v1 = v[1];
            uint32_t sum = 0xC6EF3720;
            constexpr uint32_t delta = 0x9e3779b9;
            
            for (uint32_t cycle = 0; cycle < 32; ++cycle) {
                v1 -= ((v0 << 4) + k[2]) ^ (v0 + sum) ^ ((v0 >> 5) + k[3]);
                v0 -= ((v1 << 4) + k[0]) ^ (v1 + sum) ^ ((v1 >> 5) + k[1]);
                sum -= delta;
            }
            
            v[0] = v0;
            v[1] = v1;
            std::memcpy(buffer.subspan(offset, BlockSize).data(), v, BlockSize);
        }

        return true;
    }
};
