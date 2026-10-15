#include "DynamicRingVertexBuffer.h"
#include <cstring>

namespace EterLib::Render
{
    DynamicRingVertexBuffer::~DynamicRingVertexBuffer()
    {
        Reset();
        safe_release(m_device);
    }

    DynamicRingVertexBuffer::DynamicRingVertexBuffer(DynamicRingVertexBuffer&& other) noexcept
        : m_device(other.m_device)
        , m_buffer(other.m_buffer)
        , m_sizeBytes(other.m_sizeBytes)
        , m_currentOffset(other.m_currentOffset)
    {
        other.m_device = nullptr;
        other.m_buffer = nullptr;
        other.m_sizeBytes = 0;
        other.m_currentOffset = 0;
    }

    DynamicRingVertexBuffer& DynamicRingVertexBuffer::operator=(DynamicRingVertexBuffer&& other) noexcept
    {
        if (this != &other)
        {
            Reset();
            safe_release(m_device);

            m_device = other.m_device;
            m_buffer = other.m_buffer;
            m_sizeBytes = other.m_sizeBytes;
            m_currentOffset = other.m_currentOffset;

            other.m_device = nullptr;
            other.m_buffer = nullptr;
            other.m_sizeBytes = 0;
            other.m_currentOffset = 0;
        }
        return *this;
    }

    bool DynamicRingVertexBuffer::Initialize(LPDIRECT3DDEVICE9 dev, size_t sizeBytes)
    {
        if (!dev || sizeBytes == 0)
        {
            EterBase::ModernLogger::Error("DynamicRingVertexBuffer::Initialize failed: Invalid arguments.");
            return false;
        }

        Reset();

        if (m_device != dev)
        {
            safe_release(m_device);
            m_device = dev;
            if (m_device) m_device->AddRef();
        }

        m_sizeBytes = sizeBytes;
        m_currentOffset = 0;

        HRESULT hr = m_device->CreateVertexBuffer(
            static_cast<UINT>(m_sizeBytes),
            D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
            0, // No FVF, we assume vertex declaration is handled externally
            D3DPOOL_DEFAULT,
            &m_buffer,
            nullptr
        );

        if (FAILED(hr))
        {
            EterBase::ModernLogger::Error("DynamicRingVertexBuffer::Initialize failed to create vertex buffer.");
            Reset();
            safe_release(m_device);
            return false;
        }

        EterBase::ModernLogger::Info("DynamicRingVertexBuffer initialized with size {} bytes.", m_sizeBytes);
        return true;
    }

    uint32_t DynamicRingVertexBuffer::Allocate(size_t sizeBytes, const void* data, uint32_t stride)
    {
        if (!m_buffer || sizeBytes == 0 || !data || sizeBytes > m_sizeBytes || stride == 0)
        {
            return 0xFFFFFFFF;
        }

        // Align current offset to stride
        size_t remainder = m_currentOffset % stride;
        if (remainder != 0)
        {
            m_currentOffset += (stride - remainder);
        }

        DWORD lockFlags = D3DLOCK_NOOVERWRITE;

        // Check if wrap-around is needed
        if (m_currentOffset + sizeBytes > m_sizeBytes)
        {
            m_currentOffset = 0;
            lockFlags = D3DLOCK_DISCARD;
        }

        void* lockData = nullptr;
        HRESULT hr = m_buffer->Lock(
            static_cast<UINT>(m_currentOffset),
            static_cast<UINT>(sizeBytes),
            &lockData,
            lockFlags
        );

        if (FAILED(hr) || !lockData)
        {
            EterBase::ModernLogger::Error("DynamicRingVertexBuffer::Allocate failed to lock vertex buffer.");
            return 0xFFFFFFFF;
        }

        std::memcpy(lockData, data, sizeBytes);
        m_buffer->Unlock();

        uint32_t allocatedOffset = static_cast<uint32_t>(m_currentOffset);
        m_currentOffset += sizeBytes;

        return allocatedOffset;
    }

    LPDIRECT3DVERTEXBUFFER9 DynamicRingVertexBuffer::GetBuffer() const noexcept
    {
        return m_buffer;
    }

    void DynamicRingVertexBuffer::Reset() noexcept
    {
        if (m_buffer)
        {
            safe_release(m_buffer);
        }
        m_currentOffset = 0;
    }

} // namespace EterLib::Render

