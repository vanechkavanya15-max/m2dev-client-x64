#pragma once

#include <cstdint>
#include <span>

namespace Client::Graphics::RHI
{

enum class RHITextureFormat : uint8_t
{
    R8G8B8A8_UNORM,
    DXT1,
    DXT3,
    DXT5,
    D24S8
};

enum class RHISamplerFilter : uint8_t
{
    Point,
    Linear,
    Anisotropic
};

enum class RHIAddressMode : uint8_t
{
    Wrap,
    Clamp,
    Mirror,
    Border
};

class IRHITexture
{
public:
    virtual ~IRHITexture() = default;

    [[nodiscard]] virtual uint32_t GetWidth() const noexcept = 0;
    [[nodiscard]] virtual uint32_t GetHeight() const noexcept = 0;
    [[nodiscard]] virtual uint32_t GetMipLevels() const noexcept = 0;
    [[nodiscard]] virtual RHITextureFormat GetFormat() const noexcept = 0;

    virtual bool UpdateSubresource(uint32_t mipLevel, std::span<const uint8_t> pixelData, size_t pitch) = 0;
};

class IRHISampler
{
public:
    virtual ~IRHISampler() = default;

    [[nodiscard]] virtual RHISamplerFilter GetFilter() const noexcept = 0;
    [[nodiscard]] virtual RHIAddressMode GetAddressU() const noexcept = 0;
    [[nodiscard]] virtual RHIAddressMode GetAddressV() const noexcept = 0;
};

} // namespace Client::Graphics::RHI
