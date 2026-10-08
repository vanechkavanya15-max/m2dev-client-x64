#pragma once

#include <cstdint>
#include <compare>
#include <algorithm>

namespace EterLib::Render
{
    struct RenderSortKey
    {
        uint64_t value{0};

        constexpr auto operator<=>(const RenderSortKey&) const = default;
    };

    class SortKeyBuilder
    {
    public:
        constexpr SortKeyBuilder() noexcept = default;

        // O(1) fluent methods
        // Layout (64 bits total):
        // 56-63: Pass (8 bits)
        // 52-55: Viewport (4 bits)
        // 40-51: Depth (12 bits)
        // 24-39: Shader (16 bits)
        // 16-23: Material (8 bits)
        //  0-15: Texture (16 bits)

        constexpr SortKeyBuilder& WithPass(uint8_t passId) noexcept
        {
            m_key = (m_key & ~(0xFFULL << 56)) | ((static_cast<uint64_t>(passId) & 0xFFULL) << 56);
            return *this;
        }

        constexpr SortKeyBuilder& WithViewport(uint8_t vpId) noexcept
        {
            m_key = (m_key & ~(0xFULL << 52)) | ((static_cast<uint64_t>(vpId) & 0xFULL) << 52);
            return *this;
        }

        constexpr SortKeyBuilder& WithDepth(float normalizedDepth, bool reverseForTransparent = false) noexcept
        {
            float clamped = normalizedDepth;
            if (clamped < 0.0f) clamped = 0.0f;
            if (clamped > 1.0f) clamped = 1.0f;
            
            // 12-bit float to int quantization
            uint32_t depthInt = static_cast<uint32_t>(clamped * 4095.0f);
            
            if (reverseForTransparent)
            {
                depthInt = 4095 - depthInt;
            }
            
            m_key = (m_key & ~(0xFFFULL << 40)) | ((static_cast<uint64_t>(depthInt) & 0xFFFULL) << 40);
            return *this;
        }

        constexpr SortKeyBuilder& WithShader(uint16_t shaderId) noexcept
        {
            m_key = (m_key & ~(0xFFFFULL << 24)) | ((static_cast<uint64_t>(shaderId) & 0xFFFFULL) << 24);
            return *this;
        }

        constexpr SortKeyBuilder& WithMaterial(uint8_t materialId) noexcept
        {
            m_key = (m_key & ~(0xFFULL << 16)) | ((static_cast<uint64_t>(materialId) & 0xFFULL) << 16);
            return *this;
        }

        constexpr SortKeyBuilder& WithTexture(uint32_t textureId) noexcept
        {
            m_key = (m_key & ~0xFFFFULL) | (static_cast<uint64_t>(textureId) & 0xFFFFULL);
            return *this;
        }

        constexpr RenderSortKey Build() const noexcept
        {
            return RenderSortKey{m_key};
        }

    private:
        // CPU Cache optimization: storing internal state directly as a packed 64-bit integer
        // rather than multiple members. This takes 8 bytes instead of 32+ bytes.
        uint64_t m_key{0};
    };
}

