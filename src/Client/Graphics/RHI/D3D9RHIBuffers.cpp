#include "D3D9RHIBuffers.h"

namespace Client::Graphics::RHI
{

bool D3D9RHIVertexBuffer::Create(IDirect3DDevice9* device, uint32_t sizeBytes, uint32_t stride, RHIResourceUsage usage)
{
    (void)usage;
    m_size = sizeBytes;
    m_stride = stride;

#if defined(_WIN32) || defined(_WIN64)
    if (!device)
    {
        EterBase::ModernLogger::Error("D3D9RHIVertexBuffer::Create - Null device pointer");
        return false;
    }

    DWORD d3dUsage = (usage == RHIResourceUsage::Dynamic) ? (D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY) : D3DUSAGE_WRITEONLY;
    D3DPOOL pool = (usage == RHIResourceUsage::Dynamic) ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;

    HRESULT hr = device->CreateVertexBuffer(sizeBytes, d3dUsage, 0, pool, m_buffer.ReleaseAndGetAddressOf(), nullptr);
    if (FAILED(hr))
    {
        EterBase::ModernLogger::Error("D3D9RHIVertexBuffer::Create - CreateVertexBuffer failed: 0x{:08X}", static_cast<uint32_t>(hr));
        return false;
    }
#else
    (void)device;
#endif
    return true;
}

bool D3D9RHIVertexBuffer::UpdateData(std::span<const uint8_t> data, uint32_t offset)
{
#if defined(_WIN32) || defined(_WIN64)
    if (!m_buffer)
        return false;

    if (offset + data.size() > m_size)
    {
        EterBase::ModernLogger::Error("D3D9RHIVertexBuffer::UpdateData - Overflow: offset {} + size {} > total {}", offset, data.size(), m_size);
        return false;
    }

    void* ptr = nullptr;
    DWORD flags = (offset == 0 && data.size() == m_size) ? D3DLOCK_DISCARD : 0;
    HRESULT hr = m_buffer->Lock(offset, static_cast<UINT>(data.size()), &ptr, flags);
    if (FAILED(hr) || !ptr)
        return false;

    std::memcpy(ptr, data.data(), data.size());
    m_buffer->Unlock();
    return true;
#else
    (void)data; (void)offset;
    return true;
#endif
}

bool D3D9RHIIndexBuffer::Create(IDirect3DDevice9* device, size_t indexCount, RHIIndexFormat format, RHIResourceUsage usage)
{
    (void)usage;
    m_indexCount = indexCount;
    m_format = format;
    size_t indexSize = (format == RHIIndexFormat::UInt32) ? 4 : 2;
    size_t totalBytes = indexCount * indexSize;

#if defined(_WIN32) || defined(_WIN64)
    if (!device)
    {
        EterBase::ModernLogger::Error("D3D9RHIIndexBuffer::Create - Null device pointer");
        return false;
    }

    D3DFORMAT d3dFormat = (format == RHIIndexFormat::UInt32) ? D3DFMT_INDEX32 : D3DFMT_INDEX16;
    DWORD d3dUsage = (usage == RHIResourceUsage::Dynamic) ? (D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY) : D3DUSAGE_WRITEONLY;
    D3DPOOL pool = (usage == RHIResourceUsage::Dynamic) ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;

    HRESULT hr = device->CreateIndexBuffer(static_cast<UINT>(totalBytes), d3dUsage, d3dFormat, pool, m_buffer.ReleaseAndGetAddressOf(), nullptr);
    if (FAILED(hr))
    {
        EterBase::ModernLogger::Error("D3D9RHIIndexBuffer::Create - CreateIndexBuffer failed: 0x{:08X}", static_cast<uint32_t>(hr));
        return false;
    }
#else
    (void)device;
#endif
    return true;
}

bool D3D9RHIIndexBuffer::UpdateData(std::span<const uint8_t> data, uint32_t offset)
{
#if defined(_WIN32) || defined(_WIN64)
    if (!m_buffer)
        return false;

    size_t indexSize = (m_format == RHIIndexFormat::UInt32) ? 4 : 2;
    size_t totalBytes = m_indexCount * indexSize;

    if (offset + data.size() > totalBytes)
    {
        EterBase::ModernLogger::Error("D3D9RHIIndexBuffer::UpdateData - Overflow: offset {} + size {} > total {}", offset, data.size(), totalBytes);
        return false;
    }

    void* ptr = nullptr;
    DWORD flags = (offset == 0 && data.size() == totalBytes) ? D3DLOCK_DISCARD : 0;
    HRESULT hr = m_buffer->Lock(offset, static_cast<UINT>(data.size()), &ptr, flags);
    if (FAILED(hr) || !ptr)
        return false;

    std::memcpy(ptr, data.data(), data.size());
    m_buffer->Unlock();
    return true;
#else
    (void)data; (void)offset;
    return true;
#endif
}

} // namespace Client::Graphics::RHI
