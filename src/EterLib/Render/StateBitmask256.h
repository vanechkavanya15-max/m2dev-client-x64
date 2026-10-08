#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include <immintrin.h>

namespace EterLib::Render
{
    class StateBitmask256
    {
    public:
        StateBitmask256() noexcept : m_words{0, 0, 0, 0} {}

        void SetBit(size_t index) noexcept
        {
            if (index < 256)
            {
                m_words[index / 64] |= (1ULL << (index % 64));
            }
        }

        void ClearBit(size_t index) noexcept
        {
            if (index < 256)
            {
                m_words[index / 64] &= ~(1ULL << (index % 64));
            }
        }

        [[nodiscard]] bool TestBit(size_t index) const noexcept
        {
            if (index < 256)
            {
                return (m_words[index / 64] & (1ULL << (index % 64))) != 0;
            }
            return false;
        }

        void ClearAll() noexcept
        {
            m_words = {0, 0, 0, 0};
        }

        [[nodiscard]] bool IsAnyDirty() const noexcept
        {
            // Use AVX2 SIMD intrinsic for fast testing if any bit is set (if supported by compilation flags).
            // _mm256_testz_si256 returns 1 if all bits are 0, 0 otherwise.
            __m256i mask = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(m_words.data()));
            return _mm256_testz_si256(mask, mask) == 0;
        }

    private:
        alignas(32) std::array<uint64_t, 4> m_words;
    };
} // namespace EterLib::Render
