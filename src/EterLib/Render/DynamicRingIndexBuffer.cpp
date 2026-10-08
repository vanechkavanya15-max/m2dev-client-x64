#ifndef TEST_MODE_DISABLE_STDAFX
#include "../../StdAfx.h"
#endif

#include "DynamicRingIndexBuffer.h"
#include <cstring>

namespace EterLib::Render {

DynamicRingIndexBuffer::DynamicRingIndexBuffer()
    : m_device(nullptr)
    , m_indexBuffer(nullptr)
    , m_bufferSizeBytes(0)
    , m_format(0) // Assuming 0 is a safe invalid/uninitialized format
    , m_indexSize(0)
    , m_currentOffsetBytes(0)
{
}

DynamicRingIndexBuffer::~DynamicRingIndexBuffer()
{
    Destroy();
}

void DynamicRingIndexBuffer::Destroy()
{
    if (m_indexBuffer) {
        m_indexBuffer->Release();
        m_indexBuffer = nullptr;
    }
    m_device = nullptr;
    m_bufferSizeBytes = 0;
    m_indexSize = 0;
    m_currentOffsetBytes = 0;
}

bool DynamicRingIndexBuffer::Initialize(LPDIRECT3DDEVICE9 dev, size_t sizeBytes, D3DFORMAT format)
{
    if (!dev) return false;
    
    Destroy();

    m_device = dev;
    m_bufferSizeBytes = sizeBytes;
    m_format = format;
    
    if (format == D3DFMT_INDEX16) {
        m_indexSize = 2;
    } else if (format == D3DFMT_INDEX32) {
        m_indexSize = 4;
    } else {
        return false; // Unsupported format
    }

    HRESULT hr = m_device->CreateIndexBuffer(
        static_cast<uint32_t>(m_bufferSizeBytes),
        D3DUSAGE_DYNAMIC,
        m_format,
        D3DPOOL_DEFAULT,
        &m_indexBuffer,
        nullptr
    );

    if (FAILED(hr)) {
        Destroy();
        return false;
    }

    m_currentOffsetBytes = 0;
    return true;
}

uint32_t DynamicRingIndexBuffer::AllocateIndices(size_t indexCount, const void* indices)
{
    if (!m_indexBuffer || !indices || indexCount == 0) {
        return 0xFFFFFFFF; // Error indicator
    }

    size_t requestedBytes = indexCount * m_indexSize;
    if (requestedBytes > m_bufferSizeBytes) {
        return 0xFFFFFFFF; // Request exceeds total buffer size
    }

    DWORD lockFlags = D3DLOCK_NOOVERWRITE;
    if (m_currentOffsetBytes + requestedBytes > m_bufferSizeBytes) {
        lockFlags = D3DLOCK_DISCARD;
        m_currentOffsetBytes = 0;
    }

    void* pData = nullptr;
    HRESULT hr = m_indexBuffer->Lock(
        static_cast<uint32_t>(m_currentOffsetBytes),
        static_cast<uint32_t>(requestedBytes),
        &pData,
        lockFlags
    );

    if (FAILED(hr) || !pData) {
        return 0xFFFFFFFF;
    }

    std::memcpy(pData, indices, requestedBytes);
    m_indexBuffer->Unlock();

    uint32_t startIndex = static_cast<uint32_t>(m_currentOffsetBytes / m_indexSize);
    m_currentOffsetBytes += requestedBytes;

    return startIndex;
}

LPDIRECT3DINDEXBUFFER9 DynamicRingIndexBuffer::GetBuffer() const noexcept
{
    return m_indexBuffer;
}

} // namespace EterLib::Render

