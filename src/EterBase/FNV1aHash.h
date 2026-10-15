#pragma once

#include <cstdint>
#include <string_view>
#include <span>

/**
 * @brief Utility class providing FNV-1a 64-bit hashing algorithm.
 * 
 * Provides static constexpr methods to compute the 64-bit FNV-1a hash
 * for both text strings and binary buffers.
 */
class FNV1aHash
{
public:
    /**
     * @brief Computes the 64-bit FNV-1a hash of a string view.
     * 
     * @param text The string view to hash.
     * @return uint64_t The 64-bit FNV-1a hash value.
     */
    static constexpr uint64_t Hash(std::string_view text) noexcept
    {
        uint64_t hash = 14695981039346656037ULL;
        for (char c : text)
        {
            hash ^= static_cast<uint8_t>(c);
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    /**
     * @brief Computes the 64-bit FNV-1a hash of a binary buffer.
     * 
     * @param data The binary span to hash.
     * @return uint64_t The 64-bit FNV-1a hash value.
     */
    static constexpr uint64_t Hash(std::span<const uint8_t> data) noexcept
    {
        uint64_t hash = 14695981039346656037ULL;
        for (uint8_t byte : data)
        {
            hash ^= byte;
            hash *= 1099511628211ULL;
        }
        return hash;
    }
};
