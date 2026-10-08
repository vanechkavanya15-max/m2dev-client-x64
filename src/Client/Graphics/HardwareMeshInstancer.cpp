#include "HardwareMeshInstancer.h"

#include <numeric>
#include <cstring>
#include <algorithm>

namespace Client::Graphics
{

HardwareMeshInstancer::HardwareMeshInstancer(
    uint32_t ringVertexCapacity,
    uint32_t ringIndexCapacity) noexcept
    : m_ringBuffer(ringVertexCapacity, ringIndexCapacity)
{
}

bool HardwareMeshInstancer::Initialize(IDirect3DDevice9* pDevice) noexcept
{
    return m_ringBuffer.Initialize(pDevice);
}

void HardwareMeshInstancer::Release() noexcept
{
    Clear();
    m_ringBuffer.Release();
}

void HardwareMeshInstancer::AddInstance(
    uint32_t meshId,
    uint32_t materialId,
    uint32_t lodIndex,
    const InstanceData& data)
{
    AddInstance(BucketKey{meshId, materialId, lodIndex}, data);
}

void HardwareMeshInstancer::AddInstance(const BucketKey& key, const InstanceData& data)
{
    m_buckets[key].push_back(data);
}

void HardwareMeshInstancer::ReserveBucket(const BucketKey& key, size_t capacity)
{
    m_buckets[key].reserve(capacity);
}

void HardwareMeshInstancer::Clear() noexcept
{
    m_buckets.clear();
}

void HardwareMeshInstancer::BeginFrame() noexcept
{
    Clear();
    m_ringBuffer.ResetFrame();
}

size_t HardwareMeshInstancer::GetPendingInstanceCount() const noexcept
{
    size_t count = 0;
    for (const auto& [key, list] : m_buckets)
    {
        count += list.size();
    }
    return count;
}

const std::vector<InstanceData>* HardwareMeshInstancer::GetBucketInstances(const BucketKey& key) const noexcept
{
    const auto it = m_buckets.find(key);
    if (it != m_buckets.end())
    {
        return &it->second;
    }
    return nullptr;
}

double HardwareMeshInstancer::GetDrawCallReductionRatio() const noexcept
{
    if (m_totalInstances == 0)
    {
        return 0.0;
    }
    if (m_totalDrawCalls >= m_totalInstances)
    {
        return 0.0;
    }
    return 1.0 - (static_cast<double>(m_totalDrawCalls) / static_cast<double>(m_totalInstances));
}

void HardwareMeshInstancer::ResetTelemetry() noexcept
{
    m_totalInstances = 0;
    m_totalDrawCalls = 0;
    m_streamFreqHistory.clear();
    m_batchDrawHistory.clear();
    m_lastFlushedInstanceData.clear();
    m_lastFlushedBytes.clear();
    m_ringBuffer.ResetTelemetry();
}

uint32_t HardwareMeshInstancer::Flush(IDirect3DDevice9* pDevice)
{
    if (m_buckets.empty())
    {
        return 0;
    }

    if (!m_ringBuffer.IsInitialized())
    {
        m_ringBuffer.Initialize(pDevice);
    }

    uint32_t drawCallsDispatched = 0;
    constexpr uint32_t kInstanceStride = static_cast<uint32_t>(sizeof(InstanceData));

    m_lastFlushedInstanceData.clear();
    m_lastFlushedBytes.clear();

    // Sortujemy buckety dla deterministycznej kolejnosci wysylania partii
    std::vector<BucketKey> sortedKeys;
    sortedKeys.reserve(m_buckets.size());
    for (const auto& [key, list] : m_buckets)
    {
        if (!list.empty())
        {
            sortedKeys.push_back(key);
        }
    }

    std::sort(sortedKeys.begin(), sortedKeys.end());

    for (const auto& key : sortedKeys)
    {
        const auto& instances = m_buckets[key];
        const uint32_t count = static_cast<uint32_t>(instances.size());
        if (count == 0)
        {
            continue;
        }

        const size_t totalBatchBytes = static_cast<size_t>(count) * kInstanceStride;

        // Krok 1: Przydzial ciaglego bloku pamieci w DynamicGeometryRingBuffer z D3DLOCK_NOOVERWRITE
        const auto alloc = m_ringBuffer.AllocateVertices(count, kInstanceStride);
        if (alloc.pData != nullptr)
        {
            std::memcpy(alloc.pData, instances.data(), totalBatchBytes);
        }
        m_ringBuffer.UnlockVertices();

        // Zapisujemy skopiowane instancje i bajty do audytu
        m_lastFlushedInstanceData.insert(m_lastFlushedInstanceData.end(), instances.begin(), instances.end());
        const auto* bytePtr = reinterpret_cast<const uint8_t*>(instances.data());
        m_lastFlushedBytes.insert(m_lastFlushedBytes.end(), bytePtr, bytePtr + totalBatchBytes);

        // Krok 2: Wyliczenie masek czestotliwosci strumieni
        const uint32_t freq0 = CalculateStream0Frequency(count);
        const uint32_t freq1 = CalculateStream1Frequency();

        // Rejestracja audytu strumieniowania dla testow i diagnostyki
        m_streamFreqHistory.push_back({0, freq0});
        m_streamFreqHistory.push_back({1, freq1});
        m_batchDrawHistory.push_back({key, count, alloc.offsetBytes, alloc.isDiscardWrap});

        // Krok 3: Konfiguracja strumieni w natywnym urzadzeniu Direct3D 9
        if (pDevice != nullptr)
        {
            // Podpiecie bufora instancji pod Strumien 1 z wyliczonym offsetem w bajtach
            pDevice->SetStreamSource(1, m_ringBuffer.GetVertexBuffer(), alloc.offsetBytes, kInstanceStride);

            // Strumien 0: Geometria siatki (D3DSTREAMSOURCE_INDEXEDDATA | count)
            pDevice->SetStreamSourceFreq(0, freq0);

            // Strumien 1: Bufor instancji (D3DSTREAMSOURCE_INSTANCEDATA | 1)
            pDevice->SetStreamSourceFreq(1, freq1);

            // Krok 4: Reset dzielnikow strumieni po zakonczeniu partii
            pDevice->SetStreamSourceFreq(0, CalculateStreamResetFrequency());
            pDevice->SetStreamSourceFreq(1, CalculateStreamResetFrequency());
            pDevice->SetStreamSource(1, nullptr, 0, 0);
        }

        // Zapisz zresetowany stan strumieniowania do historii audytu
        m_streamFreqHistory.push_back({0, CalculateStreamResetFrequency()});
        m_streamFreqHistory.push_back({1, CalculateStreamResetFrequency()});

        // Krok 5: Aktualizacja telemetrii
        m_totalInstances += count;
        ++m_totalDrawCalls;
        ++drawCallsDispatched;
    }

    // Wyczysc buforowane instancje po zakonczonym zrzucie partii
    m_buckets.clear();

    return drawCallsDispatched;
}

} // namespace Client::Graphics
