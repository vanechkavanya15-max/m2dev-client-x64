#pragma once

#include <cstdint>
#include <compare>

namespace EterLib::Render
{
    /**
     * @brief RenderSortKey
     * 64-bit struct using bitfields or direct uint64_t for fast front-to-back or back-to-front sorting.
     */
    struct RenderSortKey
    {
        uint64_t value{0};

        constexpr RenderSortKey() noexcept = default;
        constexpr RenderSortKey(uint64_t val) noexcept : value(val) {}

        [[nodiscard]] constexpr uint64_t AsUint64() const noexcept
        {
            return value;
        }

        [[nodiscard]] constexpr bool operator<(const RenderSortKey& other) const noexcept
        {
            return value < other.value;
        }

        [[nodiscard]] constexpr std::strong_ordering operator<=>(const RenderSortKey& other) const noexcept
        {
            return value <=> other.value;
        }

        [[nodiscard]] constexpr bool operator==(const RenderSortKey& other) const noexcept
        {
            return value == other.value;
        }
    };
}
