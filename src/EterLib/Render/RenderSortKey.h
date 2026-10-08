#pragma once

#include <cstdint>
#include <compare>
#include <bit>

namespace EterLib::Render
{
    /**
     * @brief RenderSortKey
     * 64-bit struct using bitfields for fast front-to-back or back-to-front sorting.
     */
    struct RenderSortKey
    {
        uint64_t depth    : 12; // 12 bits (LSB)
        uint64_t material : 8;  // 8 bits
        uint64_t texture  : 20; // 20 bits
        uint64_t shader   : 12; // 12 bits
        uint64_t viewport : 4;  // 4 bits
        uint64_t pass     : 8;  // 8 bits (MSB)

        /**
         * @brief Converts the bitfield to a 64-bit unsigned integer.
         */
        [[nodiscard]] constexpr uint64_t AsUint64() const noexcept
        {
            return std::bit_cast<uint64_t>(*this);
        }

        /**
         * @brief Less-than operator for sorting.
         */
        [[nodiscard]] constexpr bool operator<(const RenderSortKey& other) const noexcept
        {
            return AsUint64() < other.AsUint64();
        }

        /**
         * @brief Three-way comparison operator (C++20/23).
         */
        [[nodiscard]] constexpr std::strong_ordering operator<=>(const RenderSortKey& other) const noexcept
        {
            return AsUint64() <=> other.AsUint64();
        }

        /**
         * @brief Equality operator.
         */
        [[nodiscard]] constexpr bool operator==(const RenderSortKey& other) const noexcept = default;
    };
}

