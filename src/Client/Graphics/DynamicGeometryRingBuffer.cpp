#include "DynamicGeometryRingBuffer.h"

#include <cstring>
#include <algorithm>
#include <utility>

namespace Client::Graphics
{

DynamicGeometryRingBuffer::DynamicGeometryRingBuffer(
    uint32_t vertexCapacity,
    uint32_t indexCapacity) noexcept
    : m_vertexCapacity(vertexCapacity)
    , m_indexCapacity(indexCapacity)
{
}

DynamicGeometryRingBuffer::~DynamicGeometryRingBuffer()
{
    Release();
}

DynamicGeometryRingBuffer::DynamicGeometryRingBuffer(DynamicGeometryRingBuffer&& other) noexcept
    : m_pDevice(other.m_pDevice)
    , m_pVB(other.m_pVB)
    , m_pIB(other.m_pIB)
    , m_vertexCapacity(other.m_vertexCapacity)
    , m_indexCapacity(other.m_indexCapacity)
    , m_vertexOffset(other.m_vertexOffset)
    , m_indexOffset(other.m_indexOffset)
    , m_initialized(other.m_initialized)
    , m_isEmulation(other.m_isEmulation)
    , m_isVBLocked(other.m_isVBLocked)
    , m_isIBLocked(other.m_isIBLocked)
    , m_vertexEmulationBuffer(std::move(other.m_vertexEmulationBuffer))
    , m_indexEmulationBuffer(std::move(other.m_indexEmulationBuffer))
    , m_noOverwriteHits(other.m_noOverwriteHits)
    , m_discardWraps(other.m_discardWraps)
    , m_totalBytesStreamed(other.m_totalBytesStreamed)
{
    other.m_pDevice = nullptr;
    other.m_pVB = nullptr;
    other.m_pIB = nullptr;
    other.m_vertexOffset = 0;
    other.m_indexOffset = 0;
    other.m_initialized = false;
    other.m_isEmulation = false;
    other.m_isVBLocked = false;
    other.m_isIBLocked = false;
    other.m_noOverwriteHits = 0;
    other.m_discardWraps = 0;
    other.m_totalBytesStreamed = 0;
}

DynamicGeometryRingBuffer& DynamicGeometryRingBuffer::operator=(DynamicGeometryRingBuffer&& other) noexcept
{
    if (this != &other)
    {
        Release();

        m_pDevice = other.m_pDevice;
        m_pVB = other.m_pVB;
        m_pIB = other.m_pIB;
        m_vertexCapacity = other.m_vertexCapacity;
        m_indexCapacity = other.m_indexCapacity;
        m_vertexOffset = other.m_vertexOffset;
        m_indexOffset = other.m_indexOffset;
        m_initialized = other.m_initialized;
        m_isEmulation = other.m_isEmulation;
        m_isVBLocked = other.m_isVBLocked;
        m_isIBLocked = other.m_isIBLocked;
        m_vertexEmulationBuffer = std::move(other.m_vertexEmulationBuffer);
        m_indexEmulationBuffer = std::move(other.m_indexEmulationBuffer);
        m_noOverwriteHits = other.m_noOverwriteHits;
        m_discardWraps = other.m_discardWraps;
        m_totalBytesStreamed = other.m_totalBytesStreamed;

        other.m_pDevice = nullptr;
        other.m_pVB = nullptr;
        other.m_pIB = nullptr;
        other.m_vertexOffset = 0;
        other.m_indexOffset = 0;
        other.m_initialized = false;
        other.m_isEmulation = false;
        other.m_isVBLocked = false;
        other.m_isIBLocked = false;
        other.m_noOverwriteHits = 0;
        other.m_discardWraps = 0;
        other.m_totalBytesStreamed = 0;
    }
    return *this;
}

bool DynamicGeometryRingBuffer::Initialize(IDirect3DDevice9* pDevice) noexcept
{
    Release();

    m_pDevice = pDevice;
    m_vertexOffset = 0;
    m_indexOffset = 0;
    m_noOverwriteHits = 0;
    m_discardWraps = 0;
    m_totalBytesStreamed = 0;

    // Bezpieczny tryb emulacji RAM, gdy pDevice == nullptr (np. testy jednostkowe headless)
    if (!pDevice)
    {
        m_isEmulation = true;
        try
        {
            m_vertexEmulationBuffer.assign(m_vertexCapacity, 0);
            m_indexEmulationBuffer.assign(m_indexCapacity, 0);
        }
        catch (...)
        {
            m_initialized = false;
            return false;
        }

        m_initialized = true;
        return true;
    }

    // Tryb fizycznego Direct3D 9 z flaga D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY w D3DPOOL_DEFAULT
    m_isEmulation = false;

    // 1. Dynamiczny bufor wierzcholkow
    const HRESULT hrVB = pDevice->CreateVertexBuffer(
        static_cast<UINT>(m_vertexCapacity),
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        0, // FVF = 0, elastyczne uzycie z deklaracjami wierzcholkow
        D3DPOOL_DEFAULT,
        &m_pVB,
        nullptr
    );

    if (FAILED(hrVB) || !m_pVB)
    {
        Release();
        return false;
    }

    // 2. Dynamiczny bufor indeksow (16-bit D3DFMT_INDEX16)
    const HRESULT hrIB = pDevice->CreateIndexBuffer(
        static_cast<UINT>(m_indexCapacity),
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &m_pIB,
        nullptr
    );

    if (FAILED(hrIB) || !m_pIB)
    {
        Release();
        return false;
    }

    m_initialized = true;
    return true;
}

void DynamicGeometryRingBuffer::Release() noexcept
{
    UnlockVertices();
    UnlockIndices();

    if (m_pVB)
    {
        m_pVB->Release();
        m_pVB = nullptr;
    }

    if (m_pIB)
    {
        m_pIB->Release();
        m_pIB = nullptr;
    }

    m_vertexEmulationBuffer.clear();
    m_indexEmulationBuffer.clear();

    m_vertexOffset = 0;
    m_indexOffset = 0;
    m_initialized = false;
    m_isEmulation = false;
    m_pDevice = nullptr;
}

VertexAllocation DynamicGeometryRingBuffer::AllocateVertices(uint32_t vertexCount, uint32_t vertexStride) noexcept
{
    if (!m_initialized || vertexCount == 0 || vertexStride == 0)
    {
        return {};
    }

    const uint32_t requiredBytes = vertexCount * vertexStride;
    if (requiredBytes > m_vertexCapacity)
    {
        // Pojedyncza paczka przekracza cala pojemnosc bufora
        return {};
    }

    // Wyrownanie offsetu do wielokrotnosci vertexStride (wymog Direct3D 9 dla SetStreamSource i baseVertex)
    uint32_t alignedOffset = m_vertexOffset;
    const uint32_t remainder = alignedOffset % vertexStride;
    if (remainder != 0)
    {
        alignedOffset += (vertexStride - remainder);
    }

    bool isDiscardWrap = false;
    uint32_t offsetBytes = 0;
    DWORD lockFlags = D3DLOCK_NOOVERWRITE;

    if (alignedOffset + requiredBytes <= m_vertexCapacity)
    {
        // 1. Zmieszczono w buforze: Zero Pipeline Stall z flaga D3DLOCK_NOOVERWRITE
        offsetBytes = alignedOffset;
        isDiscardWrap = false;
        lockFlags = D3DLOCK_NOOVERWRITE;
        m_vertexOffset = alignedOffset + requiredBytes;
        ++m_noOverwriteHits;
    }
    else
    {
        // 2. Brak miejsca na koncu bufora: Wrap-Around do offsetu 0 z flaga D3DLOCK_DISCARD
        offsetBytes = 0;
        isDiscardWrap = true;
        lockFlags = D3DLOCK_DISCARD;
        m_vertexOffset = requiredBytes;
        ++m_discardWraps;
    }

    m_totalBytesStreamed += requiredBytes;

    const uint32_t baseVertex = offsetBytes / vertexStride;

    void* pData = nullptr;
    if (m_isEmulation || !m_pVB)
    {
        // Tryb emulacji RAM
        pData = m_vertexEmulationBuffer.data() + offsetBytes;
    }
    else
    {
        // Tryb Direct3D 9 - zamknij poprzednia blokade, jesli byla otwarta
        if (m_isVBLocked)
        {
            m_pVB->Unlock();
            m_isVBLocked = false;
        }

        void* pGpuData = nullptr;
        const HRESULT hr = m_pVB->Lock(
            static_cast<UINT>(offsetBytes),
            static_cast<UINT>(requiredBytes),
            &pGpuData,
            lockFlags
        );

        if (FAILED(hr) || !pGpuData)
        {
            return {};
        }

        m_isVBLocked = true;
        pData = pGpuData;
    }

    return VertexAllocation{
        .pData = pData,
        .baseVertex = baseVertex,
        .offsetBytes = offsetBytes,
        .isDiscardWrap = isDiscardWrap
    };
}

IndexAllocation DynamicGeometryRingBuffer::AllocateIndices(uint32_t indexCount) noexcept
{
    if (!m_initialized || indexCount == 0)
    {
        return {};
    }

    constexpr uint32_t indexStride = sizeof(uint16_t); // 2 bajty dla D3DFMT_INDEX16
    const uint32_t requiredBytes = indexCount * indexStride;
    if (requiredBytes > m_indexCapacity)
    {
        // Pojedyncza paczka indeksow przekracza cala pojemnosc bufora
        return {};
    }

    // Wyrownanie offsetu do 2 bajtow (sizeof uint16_t)
    uint32_t alignedOffset = m_indexOffset;
    if ((alignedOffset & 1) != 0)
    {
        ++alignedOffset;
    }

    bool isDiscardWrap = false;
    uint32_t offsetBytes = 0;
    DWORD lockFlags = D3DLOCK_NOOVERWRITE;

    if (alignedOffset + requiredBytes <= m_indexCapacity)
    {
        // 1. Zmieszczono w buforze: Zero Pipeline Stall z flaga D3DLOCK_NOOVERWRITE
        offsetBytes = alignedOffset;
        isDiscardWrap = false;
        lockFlags = D3DLOCK_NOOVERWRITE;
        m_indexOffset = alignedOffset + requiredBytes;
        ++m_noOverwriteHits;
    }
    else
    {
        // 2. Wrap-Around do offsetu 0 z flaga D3DLOCK_DISCARD
        offsetBytes = 0;
        isDiscardWrap = true;
        lockFlags = D3DLOCK_DISCARD;
        m_indexOffset = requiredBytes;
        ++m_discardWraps;
    }

    m_totalBytesStreamed += requiredBytes;

    const uint32_t startIndex = offsetBytes / indexStride;

    void* pData = nullptr;
    if (m_isEmulation || !m_pIB)
    {
        // Tryb emulacji RAM
        pData = m_indexEmulationBuffer.data() + offsetBytes;
    }
    else
    {
        // Tryb Direct3D 9 - zamknij poprzednia blokade, jesli byla otwarta
        if (m_isIBLocked)
        {
            m_pIB->Unlock();
            m_isIBLocked = false;
        }

        void* pGpuData = nullptr;
        const HRESULT hr = m_pIB->Lock(
            static_cast<UINT>(offsetBytes),
            static_cast<UINT>(requiredBytes),
            &pGpuData,
            lockFlags
        );

        if (FAILED(hr) || !pGpuData)
        {
            return {};
        }

        m_isIBLocked = true;
        pData = pGpuData;
    }

    return IndexAllocation{
        .pData = pData,
        .startIndex = startIndex,
        .offsetBytes = offsetBytes,
        .isDiscardWrap = isDiscardWrap
    };
}

void DynamicGeometryRingBuffer::UnlockVertices() noexcept
{
    if (m_isVBLocked && m_pVB)
    {
        m_pVB->Unlock();
    }
    m_isVBLocked = false;
}

void DynamicGeometryRingBuffer::UnlockIndices() noexcept
{
    if (m_isIBLocked && m_pIB)
    {
        m_pIB->Unlock();
    }
    m_isIBLocked = false;
}

void DynamicGeometryRingBuffer::ResetFrame() noexcept
{
    // Zamknij wszelkie otwarte blokady z biezacej ramki renderowania
    UnlockVertices();
    UnlockIndices();
}

void DynamicGeometryRingBuffer::ResetTelemetry() noexcept
{
    m_noOverwriteHits = 0;
    m_discardWraps = 0;
    m_totalBytesStreamed = 0;
}

double DynamicGeometryRingBuffer::GetStallFreeRatio() const noexcept
{
    const uint64_t total = m_noOverwriteHits + m_discardWraps;
    if (total == 0)
    {
        return 100.0;
    }
    return (static_cast<double>(m_noOverwriteHits) / static_cast<double>(total)) * 100.0;
}

} // namespace Client::Graphics
