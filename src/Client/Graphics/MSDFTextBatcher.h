#pragma once

#include "MSDFFontRenderer.h"
#include "DynamicGeometryRingBuffer.h"

#include <vector>
#include <string_view>
#include <cstdint>

struct IDirect3DDevice9;

namespace Client::Graphics
{

/**
 * @brief Struktura podsumowujaca ostatnia operacje Flush do bufora pierscieniowego.
 */
struct LastFlushInfo
{
    VertexAllocation vertexAlloc{};
    IndexAllocation indexAlloc{};
    uint32_t vertexCount{0};
    uint32_t indexCount{0};
    bool success{false};
};

/**
 * @class MSDFTextBatcher
 * @brief Agregator i batcher geometrii tekstu MSDF dla interfejsu uzytkownika i etykiet swiata (nametagi, DMG, czat).
 *
 * Agreguje wszystkie napisy w pojedynczej ramce w jedna partie wierzcholkow i indeksow,
 * alokuje pamiec w DynamicGeometryRingBuffer z uzyciem floty NOOVERWRITE / DISCARD,
 * a nastepnie wykonuje dokladnie 1 DrawIndexedPrimitive.
 */
class MSDFTextBatcher
{
public:
    explicit MSDFTextBatcher(const MSDFFontAtlas* pFontAtlas = nullptr) noexcept;
    ~MSDFTextBatcher() = default;

    MSDFTextBatcher(const MSDFTextBatcher&) = delete;
    MSDFTextBatcher& operator=(const MSDFTextBatcher&) = delete;
    MSDFTextBatcher(MSDFTextBatcher&&) noexcept = default;
    MSDFTextBatcher& operator=(MSDFTextBatcher&&) noexcept = default;

    /**
     * @brief Ustawia powiazany atlas czcionki MSDF.
     */
    void SetFontAtlas(const MSDFFontAtlas* pFontAtlas) noexcept { m_pFontAtlas = pFontAtlas; }

    /**
     * @brief Zwraca biezacy atlas czcionki.
     */
    [[nodiscard]] const MSDFFontAtlas* GetFontAtlas() const noexcept { return m_pFontAtlas; }

    /**
     * @brief Dodaje napis do biezacej partii wierzcholkow i indeksow.
     * @param text Tresc napisu (UTF-8 lub ASCII).
     * @param x Poczatkowa wspolrzedna X.
     * @param y Poczatkowa wspolrzedna Y.
     * @param z Wspolrzedna glebokosci Z (domyslnie 0.0f).
     * @param color Kolor bazowy tekstu w formacie 0xAARRGGBB lub 0xRRGGBBAA.
     * @param outlineColor Kolor obrysu (0x00000000 oznacza brak obrysu).
     * @param outlineWidth Grubosc obrysu w jednostkach odleglosci (0.0 = brak).
     * @param pxRange Zakres pola odleglosci w pikselach ekranu (domyslnie 4.0f).
     * @param scale Mnoznik skali geometrii tekstu (domyslnie 1.0f).
     */
    void AddText(
        std::string_view text,
        float x,
        float y,
        float z = 0.0f,
        uint32_t color = 0xFFFFFFFF,
        uint32_t outlineColor = 0x00000000,
        float outlineWidth = 0.0f,
        float pxRange = 4.0f,
        float scale = 1.0f);

    /**
     * @brief Alokuje zgromadzona geometrie w DynamicGeometryRingBuffer i wykonuje 1 DrawIndexedPrimitive.
     * @param pRingBuffer Wskaznik do bufora pierscieniowego geometrii.
     * @param pDevice Wskaznik do urzadzenia Direct3D 9 (nullptr uruchamia tryb emulacji/headless).
     * @return true jesli partia zostala zaalokowana i wyrenderowana, false w przeciwnym wypadku.
     */
    bool Flush(DynamicGeometryRingBuffer* pRingBuffer, IDirect3DDevice9* pDevice = nullptr);

    /**
     * @brief Resetuje stan biezacej ramki oraz telemetrie.
     */
    void ResetFrame() noexcept;

    // Metryki telemetrii
    [[nodiscard]] size_t GetBatchedStringCount() const noexcept { return m_batchedStringCount; }
    [[nodiscard]] size_t GetBatchedGlyphCount() const noexcept { return m_batchedGlyphCount; }
    [[nodiscard]] uint32_t GetDrawCallsCount() const noexcept { return m_drawCallsCount; }

    // Inspektory wierzcholkow i indeksow biezacej partii
    [[nodiscard]] size_t GetVertexCount() const noexcept { return m_vertices.size(); }
    [[nodiscard]] size_t GetIndexCount() const noexcept { return m_indices.size(); }
    [[nodiscard]] const std::vector<MSDFVertex>& GetVertices() const noexcept { return m_vertices; }
    [[nodiscard]] const std::vector<uint16_t>& GetIndices() const noexcept { return m_indices; }
    [[nodiscard]] const LastFlushInfo& GetLastFlushInfo() const noexcept { return m_lastFlushInfo; }

private:
    const MSDFFontAtlas* m_pFontAtlas{nullptr};

    std::vector<MSDFVertex> m_vertices;
    std::vector<uint16_t> m_indices;

    size_t m_batchedStringCount{0};
    size_t m_batchedGlyphCount{0};
    uint32_t m_drawCallsCount{0};

    LastFlushInfo m_lastFlushInfo{};
};

} // namespace Client::Graphics
