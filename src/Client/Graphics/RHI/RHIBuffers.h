#pragma once

#include <cstdint>
#include <span>

namespace Client::Graphics::RHI
{

enum class RHIIndexFormat : uint8_t
{
    UInt16,
    UInt32
};

enum class RHIResourceUsage : uint8_t
{
    Default,
    Dynamic,
    Immutable
};

class IRHIVertexBuffer
{
public:
    virtual ~IRHIVertexBuffer() = default;

    virtual bool UpdateData(std::span<const uint8_t> data, uint32_t offset = 0) = 0;
    virtual size_t GetSize() const noexcept = 0;
    virtual uint32_t GetStride() const noexcept = 0;
};

class IRHIIndexBuffer
{
public:
    virtual ~IRHIIndexBuffer() = default;

    virtual bool UpdateData(std::span<const uint8_t> data, uint32_t offset = 0) = 0;
    virtual size_t GetIndexCount() const noexcept = 0;
    virtual RHIIndexFormat GetFormat() const noexcept = 0;
};

} // namespace Client::Graphics::RHI
