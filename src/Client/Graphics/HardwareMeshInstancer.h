#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <functional>
#include <utility>
#include <memory>
#include <cstring>

#if defined(__has_include)
#  if __has_include(<d3d9.h>)
#    include <d3d9.h>
#  elif __has_include("d3d9.h")
#    include "d3d9.h"
#  endif
#else
#  include <d3d9.h>
#endif

#include "DynamicGeometryRingBuffer.h"

// Bezpieczne definicje stalych dzielnika czestotliwosci strumieni D3D9
#ifndef D3DSTREAMSOURCE_INDEXEDDATA
#define D3DSTREAMSOURCE_INDEXEDDATA  (1u << 30)
#endif

#ifndef D3DSTREAMSOURCE_INSTANCEDATA
#define D3DSTREAMSOURCE_INSTANCEDATA (2u << 30)
#endif

namespace Client::Graphics
{

/**
 * @brief Struktura danych pojedynczej instancji przekazywana do GPU w Strumieniu 1.
 *
 * Sklada sie dokladnie z 80 bajtow:
 * - 4 wiersze macierzy swiata 4x4 (64 bajty)
 * - 4 bajty colorTint (kolor/odcien ARGB)
 * - 4 bajty windPhase (faza wiatru)
 * - 4 bajty lodIndex (poziom szczegolowosci)
 * - 4 bajty padding (wyrownanie do 16 bajtow)
 */
#pragma pack(push, 4)
struct InstanceData
{
    float worldRow0[4]{1.0f, 0.0f, 0.0f, 0.0f};
    float worldRow1[4]{0.0f, 1.0f, 0.0f, 0.0f};
    float worldRow2[4]{0.0f, 0.0f, 1.0f, 0.0f};
    float worldRow3[4]{0.0f, 0.0f, 0.0f, 1.0f};
    uint32_t colorTint{0xFFFFFFFFu};
    float windPhase{0.0f};
    uint32_t lodIndex{0u};
    float padding{0.0f};

    constexpr bool operator==(const InstanceData& other) const noexcept
    {
        return colorTint == other.colorTint &&
               lodIndex == other.lodIndex &&
               windPhase == other.windPhase &&
               worldRow0[0] == other.worldRow0[0] &&
               worldRow0[1] == other.worldRow0[1] &&
               worldRow0[2] == other.worldRow0[2] &&
               worldRow0[3] == other.worldRow0[3] &&
               worldRow1[0] == other.worldRow1[0] &&
               worldRow1[1] == other.worldRow1[1] &&
               worldRow1[2] == other.worldRow1[2] &&
               worldRow1[3] == other.worldRow1[3] &&
               worldRow2[0] == other.worldRow2[0] &&
               worldRow2[1] == other.worldRow2[1] &&
               worldRow2[2] == other.worldRow2[2] &&
               worldRow2[3] == other.worldRow2[3] &&
               worldRow3[0] == other.worldRow3[0] &&
               worldRow3[1] == other.worldRow3[1] &&
               worldRow3[2] == other.worldRow3[2] &&
               worldRow3[3] == other.worldRow3[3];
    }
};
#pragma pack(pop)

static_assert(sizeof(InstanceData) == 80, "Rozmiar InstanceData musi wynosic dokladnie 80 bajtow");

/**
 * @brief Klucz identyfikujacy unikalna partie rysowania (bucket) siatki statycznej.
 */
struct BucketKey
{
    uint32_t meshId{0};
    uint32_t materialId{0};
    uint32_t lodIndex{0};

    constexpr bool operator==(const BucketKey& other) const noexcept
    {
        return meshId == other.meshId &&
               materialId == other.materialId &&
               lodIndex == other.lodIndex;
    }

    constexpr bool operator<(const BucketKey& other) const noexcept
    {
        if (meshId != other.meshId)
            return meshId < other.meshId;
        if (materialId != other.materialId)
            return materialId < other.materialId;
        return lodIndex < other.lodIndex;
    }
};

/**
 * @brief Rekord wywolania SetStreamSourceFreq do audytu telemetrii i weryfikacji w testach.
 */
struct StreamFreqCallRecord
{
    uint32_t streamIndex{0};
    uint32_t frequencySetting{0};
};

/**
 * @brief Rekord partii przeslanej do bufora instancji w pamieci GPU/emulacji.
 */
struct InstancedBatchDrawRecord
{
    BucketKey key;
    uint32_t instanceCount{0};
    uint32_t bufferOffsetBytes{0};
    bool isDiscardWrap{false};
};

} // namespace Client::Graphics

// Specjalizacja std::hash dla BucketKey PRZED uzyciem w std::unordered_map
namespace std
{
template <>
struct hash<Client::Graphics::BucketKey>
{
    size_t operator()(const Client::Graphics::BucketKey& key) const noexcept
    {
        size_t h1 = std::hash<uint32_t>{}(key.meshId);
        size_t h2 = std::hash<uint32_t>{}(key.materialId);
        size_t h3 = std::hash<uint32_t>{}(key.lodIndex);

        size_t seed = h1;
        seed ^= h2 + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        seed ^= h3 + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        return seed;
    }
};
} // namespace std

namespace Client::Graphics
{

/**
 * @class HardwareMeshInstancer
 * @brief Modul zarzadzajacy sprzetowym instancingiem siatek statycznych (Direct3D 9 Hardware Mesh Instancing).
 *
 * Grupuje instancje w unikalne buckety wedlug (meshId, materialId, lodIndex),
 * przydziela pamiec w dynamicznym pierscieniowym buforze DynamicGeometryRingBuffer z flaga D3DLOCK_NOOVERWRITE,
 * konfiguruje dzielnik czestotliwosci strumieni D3D9 SetStreamSourceFreq i rejestruje pelna telemetrie redukcji DrawCalli.
 */
class HardwareMeshInstancer
{
public:
    explicit HardwareMeshInstancer(
        uint32_t ringVertexCapacity = DynamicGeometryRingBuffer::kDefaultVertexCapacity,
        uint32_t ringIndexCapacity = DynamicGeometryRingBuffer::kDefaultIndexCapacity) noexcept;

    ~HardwareMeshInstancer() = default;

    HardwareMeshInstancer(const HardwareMeshInstancer&) = delete;
    HardwareMeshInstancer& operator=(const HardwareMeshInstancer&) = delete;

    HardwareMeshInstancer(HardwareMeshInstancer&&) noexcept = default;
    HardwareMeshInstancer& operator=(HardwareMeshInstancer&&) noexcept = default;

    /**
     * @brief Inicjalizuje wewnetrzny bufor pierscieniowy instancji.
     * @param pDevice Opcjonalny wskaznik na urzadzenie D3D9 (nullptr uruchamia bezpieczny tryb emulacji RAM).
     * @return true jesli inicjalizacja powiodla sie, w przeciwnym razie false.
     */
    bool Initialize(IDirect3DDevice9* pDevice = nullptr) noexcept;

    /**
     * @brief Zwalnia zasoby i czysci kolejki instancji.
     */
    void Release() noexcept;

    /**
     * @brief Dodaje pojedyncza instancje do odpowiedniego bucketa partii.
     */
    void AddInstance(uint32_t meshId, uint32_t materialId, uint32_t lodIndex, const InstanceData& data);

    /**
     * @brief Dodaje pojedyncza instancje z jawnym kluczem BucketKey.
     */
    void AddInstance(const BucketKey& key, const InstanceData& data);

    /**
     * @brief Przedsprzedaje pojemnosc dla danego bucketa w celu redukcji realokacji wektora.
     */
    void ReserveBucket(const BucketKey& key, size_t capacity);

    /**
     * @brief Czysci wszystkie zgromadzone instancje w bucketach bez resetowania telemetrii.
     */
    void Clear() noexcept;

    /**
     * @brief Rozpoczyna nowa ramke renderowania (czysci buckety i resetuje stan bufora pierscieniowego).
     */
    void BeginFrame() noexcept;

    using MeshDrawCallback = std::function<void(const BucketKey& key, uint32_t instanceCount)>;

    /**
     * @brief Przesyla wszystkie zgromadzone partie do bufora instancji i wysyla wywolania strumieniowania D3D9.
     *
     * Dla kazdego bucketa:
     * 1. Alokuje pamiec w DynamicGeometryRingBuffer dla N instancji (N * sizeof(InstanceData)).
     * 2. Zapisuje dane instancji z flaga D3DLOCK_NOOVERWRITE (lub DISCARD przy wrapie).
     * 3. Ustawia SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | N).
     * 4. Ustawia SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1).
     * 5. Opcjonalnie wywoluje draw call jesli podano callback lub urzadzenie.
     * 6. Resetuje SetStreamSourceFreq(0, 1) i SetStreamSourceFreq(1, 1).
     *
     * @param pDevice Wskaznik do urzadzenia Direct3D 9 (nullptr uzywa trybu emulacji / audytu headless).
     * @param drawCallback Opcjonalny callback wywolywany po skonfigurowaniu strumieni do faktycznego narysowania mesha.
     * @return Liczba wyslanych partii (draw calli).
     */
    uint32_t Flush(IDirect3DDevice9* pDevice = nullptr, const MeshDrawCallback& drawCallback = nullptr);

    // Gettery stanu i kolekcji
    [[nodiscard]] size_t GetBucketCount() const noexcept { return m_buckets.size(); }
    [[nodiscard]] size_t GetPendingInstanceCount() const noexcept;
    [[nodiscard]] const std::vector<InstanceData>* GetBucketInstances(const BucketKey& key) const noexcept;
    [[nodiscard]] const std::unordered_map<BucketKey, std::vector<InstanceData>>& GetBuckets() const noexcept { return m_buckets; }

    // Telemetria wydajnosciowa
    [[nodiscard]] uint64_t GetTotalInstances() const noexcept { return m_totalInstances; }
    [[nodiscard]] uint64_t GetTotalDrawCalls() const noexcept { return m_totalDrawCalls; }
    [[nodiscard]] double GetDrawCallReductionRatio() const noexcept;
    void ResetTelemetry() noexcept;

    // Audyt strumieniowania D3D9 i testy
    [[nodiscard]] const std::vector<StreamFreqCallRecord>& GetStreamFreqHistory() const noexcept { return m_streamFreqHistory; }
    [[nodiscard]] const std::vector<InstancedBatchDrawRecord>& GetBatchDrawHistory() const noexcept { return m_batchDrawHistory; }
    void ClearStreamFreqHistory() noexcept { m_streamFreqHistory.clear(); }
    void ClearBatchDrawHistory() noexcept { m_batchDrawHistory.clear(); }

    [[nodiscard]] const std::vector<InstanceData>& GetLastFlushedInstanceData() const noexcept { return m_lastFlushedInstanceData; }
    [[nodiscard]] const std::vector<uint8_t>& GetLastFlushedBytes() const noexcept { return m_lastFlushedBytes; }

    [[nodiscard]] DynamicGeometryRingBuffer& GetRingBuffer() noexcept { return m_ringBuffer; }
    [[nodiscard]] const DynamicGeometryRingBuffer& GetRingBuffer() const noexcept { return m_ringBuffer; }

    // Pomocnicze funkcje statyczne do obliczania masek D3D9 SetStreamSourceFreq
    static constexpr uint32_t CalculateStream0Frequency(uint32_t instanceCount) noexcept
    {
        return D3DSTREAMSOURCE_INDEXEDDATA | instanceCount;
    }

    static constexpr uint32_t CalculateStream1Frequency() noexcept
    {
        return D3DSTREAMSOURCE_INSTANCEDATA | 1u;
    }

    static constexpr uint32_t CalculateStreamResetFrequency() noexcept
    {
        return 1u;
    }

private:
    std::unordered_map<BucketKey, std::vector<InstanceData>> m_buckets;
    DynamicGeometryRingBuffer m_ringBuffer;

    uint64_t m_totalInstances{0};
    uint64_t m_totalDrawCalls{0};

    std::vector<StreamFreqCallRecord> m_streamFreqHistory;
    std::vector<InstancedBatchDrawRecord> m_batchDrawHistory;

    std::vector<InstanceData> m_lastFlushedInstanceData;
    std::vector<uint8_t> m_lastFlushedBytes;
};

} // namespace Client::Graphics
