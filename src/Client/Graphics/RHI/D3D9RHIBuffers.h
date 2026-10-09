#pragma once

#include "RHIBuffers.h"
#include "EterBase/LogModern.h"

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d9.h>
#include <wrl/client.h>
#endif

struct IDirect3DDevice9;
struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;

namespace Client::Graphics::RHI
{

class D3D9RHIVertexBuffer : public IRHIVertexBuffer
{
public:
    D3D9RHIVertexBuffer() = default;
    ~D3D9RHIVertexBuffer() override = default;

    bool Create(IDirect3DDevice9* device, uint32_t sizeBytes, uint32_t stride, RHIResourceUsage usage = RHIResourceUsage::Default);
    bool UpdateData(std::span<const uint8_t> data, uint32_t offset = 0) override;

    [[nodiscard]] size_t GetSize() const noexcept override { return m_size; }
    [[nodiscard]] uint32_t GetStride() const noexcept override { return m_stride; }

#if defined(_WIN32) || defined(_WIN64)
    [[nodiscard]] IDirect3DVertexBuffer9* GetD3D9Buffer() const noexcept { return m_buffer.Get(); }
#endif

private:
    size_t m_size = 0;
    uint32_t m_stride = 0;
#if defined(_WIN32) || defined(_WIN64)
    Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9> m_buffer;
#endif
};

class D3D9RHIIndexBuffer : public IRHIIndexBuffer
{
public:
    D3D9RHIIndexBuffer() = default;
    ~D3D9RHIIndexBuffer() override = default;

    bool Create(IDirect3DDevice9* device, size_t indexCount, RHIIndexFormat format, RHIResourceUsage usage = RHIResourceUsage::Default);
    bool UpdateData(std::span<const uint8_t> data, uint32_t offset = 0) override;

    [[nodiscard]] size_t GetIndexCount() const noexcept override { return m_indexCount; }
    [[nodiscard]] RHIIndexFormat GetFormat() const noexcept override { return m_format; }

#if defined(_WIN32) || defined(_WIN64)
    [[nodiscard]] IDirect3DIndexBuffer9* GetD3D9Buffer() const noexcept { return m_buffer.Get(); }
#endif

private:
    size_t m_indexCount = 0;
    RHIIndexFormat m_format = RHIIndexFormat::UInt16;
#if defined(_WIN32) || defined(_WIN64)
    Microsoft::WRL::ComPtr<IDirect3DIndexBuffer9> m_buffer;
#endif
};

} // namespace Client::Graphics::RHI
