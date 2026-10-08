#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

#include <d3d9.h>

namespace Client::Graphics

{

/**
 * @brief Struktura opisujaca przydzial pamieci w dynamicznym pierscieniowym buforze wierzcholkow.
 */
struct VertexAllocation
{
    void* pData{nullptr};
    uint32_t baseVertex{0};
    uint32_t offsetBytes{0};
    bool isDiscardWrap{false};
};

/**
 * @brief Struktura opisujaca przydzial pamieci w dynamicznym pierscieniowym buforze indeksow.
 */
struct IndexAllocation
{
    void* pData{nullptr};
    uint32_t startIndex{0};
    uint32_t offsetBytes{0};
    bool isDiscardWrap{false};
};

/**
 * @class DynamicGeometryRingBuffer
 * @brief Dynamiczny pierscieniowy bufor geometrii (Vertex Buffer + Index Buffer) dla Direct3D 9 w standardzie C++23.
 *
 * Implementuje wzorzec Ring Buffer ze strumieniowaniem Zero Pipeline Stall:
 * - Alokacje mieszczace sie w buforze sa blokowane flaga D3DLOCK_NOOVERWRITE.
 * - Przy przekroczeniu pojemnosci nastepuje wrap-around do offsetu 0 z flaga D3DLOCK_DISCARD.
 * - Obsluguje w pelni bezpieczny tryb emulacji RAM, gdy pDevice == nullptr (np. w testach jednostkowych headless).
 */
class DynamicGeometryRingBuffer
{
public:
    static constexpr uint32_t kDefaultVertexCapacity = 8 * 1024 * 1024; // 8 MB
    static constexpr uint32_t kDefaultIndexCapacity  = 2 * 1024 * 1024; // 2 MB

    explicit DynamicGeometryRingBuffer(
        uint32_t vertexCapacity = kDefaultVertexCapacity,
        uint32_t indexCapacity = kDefaultIndexCapacity) noexcept;

    ~DynamicGeometryRingBuffer();

    // Zakaz kopiowania (zasoby D3D i unikalny stan strumieniowania)
    DynamicGeometryRingBuffer(const DynamicGeometryRingBuffer&) = delete;
    DynamicGeometryRingBuffer& operator=(const DynamicGeometryRingBuffer&) = delete;

    // Przenoszenie (Move semantics)
    DynamicGeometryRingBuffer(DynamicGeometryRingBuffer&& other) noexcept;
    DynamicGeometryRingBuffer& operator=(DynamicGeometryRingBuffer&& other) noexcept;

    /**
     * @brief Inicjalizuje bufor pierscieniowy.
     * @param pDevice Wskaznik do urzadzenia Direct3D 9 (nullptr uruchamia tryb emulacji RAM).
     * @return true jesli inicjalizacja zakonczyla sie sukcesem, w przeciwnym razie false.
     */
    bool Initialize(IDirect3DDevice9* pDevice = nullptr) noexcept;

    /**
     * @brief Zwalnia zasoby Direct3D i bufory emulacji.
     */
    void Release() noexcept;

    /**
     * @brief Alokuje ciagly blok wierzcholkow w pierscieniu.
     * @param vertexCount Liczba wierzcholkow do zaalokowania.
     * @param vertexStride Rozmiar pojedynczego wierzcholka w bajtach.
     * @return Struktura VertexAllocation ze wskaznikiem do pamieci i metadanymi.
     */
    VertexAllocation AllocateVertices(uint32_t vertexCount, uint32_t vertexStride) noexcept;

    /**
     * @brief Alokuje ciagly blok indeksow (16-bitowych) w pierscieniu.
     * @param indexCount Liczba indeksow do zaalokowania.
     * @return Struktura IndexAllocation ze wskaznikiem do pamieci i metadanymi.
     */
    IndexAllocation AllocateIndices(uint32_t indexCount) noexcept;

    /**
     * @brief Odblokowuje bufor wierzcholkow po zakonczeniu zapisu danych.
     */
    void UnlockVertices() noexcept;

    /**
     * @brief Odblokowuje bufor indeksow po zakonczeniu zapisu danych.
     */
    void UnlockIndices() noexcept;

    /**
     * @brief Resetuje stan ramki (odblokowuje ewentualne otwarte blokady i aktualizuje liczniki ramki).
     */
    void ResetFrame() noexcept;

    /**
     * @brief Resetuje liczniki telemetrii strumieniowania.
     */
    void ResetTelemetry() noexcept;

    // Telemetria wydajnosciowa
    [[nodiscard]] uint64_t GetNoOverwriteHits() const noexcept { return m_noOverwriteHits; }
    [[nodiscard]] uint64_t GetDiscardWraps() const noexcept { return m_discardWraps; }
    [[nodiscard]] uint64_t GetTotalBytesStreamed() const noexcept { return m_totalBytesStreamed; }
    [[nodiscard]] double GetStallFreeRatio() const noexcept;

    // Pobieranie natywnych zasobow Direct3D 9
    [[nodiscard]] IDirect3DVertexBuffer9* GetVertexBuffer() const noexcept { return m_pVB; }
    [[nodiscard]] IDirect3DIndexBuffer9* GetIndexBuffer() const noexcept { return m_pIB; }

    // Inspektory stanu
    [[nodiscard]] bool IsInitialized() const noexcept { return m_initialized; }
    [[nodiscard]] bool IsEmulationMode() const noexcept { return m_isEmulation; }
    [[nodiscard]] uint32_t GetVertexCapacity() const noexcept { return m_vertexCapacity; }
    [[nodiscard]] uint32_t GetIndexCapacity() const noexcept { return m_indexCapacity; }
    [[nodiscard]] uint32_t GetVertexOffset() const noexcept { return m_vertexOffset; }
    [[nodiscard]] uint32_t GetIndexOffset() const noexcept { return m_indexOffset; }

private:
    IDirect3DDevice9* m_pDevice{nullptr};
    IDirect3DVertexBuffer9* m_pVB{nullptr};
    IDirect3DIndexBuffer9* m_pIB{nullptr};

    uint32_t m_vertexCapacity{kDefaultVertexCapacity};
    uint32_t m_indexCapacity{kDefaultIndexCapacity};

    uint32_t m_vertexOffset{0};
    uint32_t m_indexOffset{0};

    bool m_initialized{false};
    bool m_isEmulation{false};
    bool m_isVBLocked{false};
    bool m_isIBLocked{false};

    // Bufory pamieci RAM uzywane w bezpiecznym trybie emulacji / testach headless
    std::vector<uint8_t> m_vertexEmulationBuffer;
    std::vector<uint8_t> m_indexEmulationBuffer;

    // Telemetria strumieniowania Zero Pipeline Stall
    uint64_t m_noOverwriteHits{0};
    uint64_t m_discardWraps{0};
    uint64_t m_totalBytesStreamed{0};
};

} // namespace Client::Graphics
