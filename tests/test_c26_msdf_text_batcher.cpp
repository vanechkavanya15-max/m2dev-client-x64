#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#if __has_include("Client/Graphics/MSDFFontRenderer.h")
#include "Client/Graphics/MSDFFontRenderer.h"
#include "Client/Graphics/MSDFTextBatcher.h"
#include "Client/Graphics/DynamicGeometryRingBuffer.h"
#else
#include "Graphics/MSDFFontRenderer.h"
#include "Graphics/MSDFTextBatcher.h"
#include "Graphics/DynamicGeometryRingBuffer.h"
#endif

#include <string>
#include <vector>

using namespace Client::Graphics;

TEST_CASE("MSDF - Test 1: Poprawnosc ukladu metryk glifow")
{
    MSDFFontAtlas atlas;
    CHECK(atlas.GetGlyphCount() == 0);

    // Ladowanie domyslnych metryk ASCII
    atlas.LoadDefaultAsciiMetrics(512.0f, 512.0f, 16.0f);
    CHECK(atlas.GetGlyphCount() >= 95);
    CHECK(atlas.GetTextureWidth() == 512.0f);
    CHECK(atlas.GetTextureHeight() == 512.0f);
    CHECK(atlas.GetLineHeight() > 0.0f);
    CHECK(atlas.GetBaseLine() > 0.0f);
    CHECK(atlas.GetDefaultPxRange() == 4.0f);

    // Sprawdzenie poprawnosci kluczowych glifow
    const char32_t sampleChars[] = { U'A', U'Z', U'a', U'z', U'0', U'9', U'!', U'?', U'+', U'-' };
    for (char32_t ch : sampleChars)
    {
        CHECK(atlas.HasGlyph(ch));
        const auto* metric = atlas.GetGlyph(ch);
        REQUIRE(metric != nullptr);
        CHECK(metric->u0 >= 0.0f);
        CHECK(metric->v0 >= 0.0f);
        CHECK(metric->u1 <= 1.0f);
        CHECK(metric->v1 <= 1.0f);
        CHECK(metric->u0 < metric->u1);
        CHECK(metric->v0 < metric->v1);
        CHECK(metric->width > 0.0f);
        CHECK(metric->height > 0.0f);
        CHECK(metric->advanceX > 0.0f);
    }

    // Sprawdzenie spacji (ASCII 32)
    CHECK(atlas.HasGlyph(U' '));
    const auto* spaceMetric = atlas.GetGlyph(U' ');
    REQUIRE(spaceMetric != nullptr);
    CHECK(spaceMetric->width == 0.0f);
    CHECK(spaceMetric->height == 0.0f);
    CHECK(spaceMetric->advanceX > 0.0f);

    // Sprawdzenie obliczania rozmiaru tekstu jednolinijkowego
    const auto dimSingle = atlas.CalculateTextSize("Metin2 x64 2026");
    CHECK(dimSingle.width > 0.0f);
    CHECK(dimSingle.height == doctest::Approx(atlas.GetLineHeight()));

    // Sprawdzenie obliczania tekstu wielolinijkowego (\n)
    const auto dimMulti = atlas.CalculateTextSize("Wojownik\nNinja\nSura\nSzaman");
    CHECK(dimMulti.width > 0.0f);
    CHECK(dimMulti.height == doctest::Approx(atlas.GetLineHeight() * 4.0f));

    // Rejestracja wlasnego glifu rozszerzonego (np. ikona miecza)
    const GlyphMetric customMetric{
        .u0 = 0.1f,
        .v0 = 0.2f,
        .u1 = 0.3f,
        .v1 = 0.4f,
        .width = 24.0f,
        .height = 24.0f,
        .offsetX = 1.0f,
        .offsetY = -2.0f,
        .advanceX = 26.0f
    };
    CHECK(atlas.RegisterGlyph(0x2694 /* skrzyzowane miecze */, customMetric));
    CHECK(atlas.HasGlyph(0x2694));
    const auto* customRetrieved = atlas.GetGlyph(0x2694);
    REQUIRE(customRetrieved != nullptr);
    CHECK(customRetrieved->width == 24.0f);
    CHECK(customRetrieved->height == 24.0f);
    CHECK(customRetrieved->offsetX == 1.0f);
    CHECK(customRetrieved->offsetY == -2.0f);
    CHECK(customRetrieved->advanceX == 26.0f);

    // Sprawdzenie obslugi fallback dla nieznanego znaku
    atlas.SetFallbackCodepoint(U'?');
    const auto* fallback = atlas.GetGlyph(0x1F9DF /* zombi */);
    REQUIRE(fallback != nullptr);
    CHECK(fallback == atlas.GetGlyph(U'?'));
}

TEST_CASE("MSDF - Test 2: Rysowanie 100 napisow (nicki, DMG popupy, chat) – weryfikacja ze wszystkie trafiaja do 1 partii")
{
    MSDFFontAtlas atlas;
    atlas.LoadDefaultAsciiMetrics(512.0f, 512.0f, 16.0f);

    MSDFTextBatcher batcher(&atlas);

    // Stan poczatkowy
    CHECK(batcher.GetBatchedStringCount() == 0);
    CHECK(batcher.GetBatchedGlyphCount() == 0);
    CHECK(batcher.GetDrawCallsCount() == 0);
    CHECK(batcher.GetVertexCount() == 0);
    CHECK(batcher.GetIndexCount() == 0);

    // Generujemy dokladnie 100 napisow reprezentujacych typowa ramke w grze:
    // 1. Etykiety graczy / nicki (30)
    for (int i = 0; i < 30; ++i)
    {
        std::string nick = "[Lv." + std::to_string(70 + i) + "] Gracz_" + std::to_string(i);
        const uint32_t nameColor = 0xFFFFFF00; // Zolty
        const uint32_t outlineColor = 0xFF000000; // Czarny obrys
        batcher.AddText(nick, 100.0f + static_cast<float>(i * 10), 150.0f, 0.0f, nameColor, outlineColor, 0.2f);
    }

    // 2. Popupy obrazen DMG (40)
    for (int i = 0; i < 40; ++i)
    {
        std::string dmgText = "-" + std::to_string(1000 + i * 250) + " CRIT";
        const uint32_t dmgColor = (i % 2 == 0) ? 0xFFFF2222 : 0xFFFF8800; // Czerwony / Pomaranczowy
        batcher.AddText(dmgText, 200.0f, 100.0f + static_cast<float>(i * 5), 0.1f, dmgColor, 0xFF000000, 0.3f);
    }

    // 3. Wiadomosci czatu gry (30)
    for (int i = 0; i < 30; ++i)
    {
        std::string chatLine = "[Czat " + std::to_string(i) + "] Witamy w Metin2 x64 Client 2026!";
        const uint32_t chatColor = 0xFFFFFFFF;
        batcher.AddText(chatLine, 10.0f, 600.0f + static_cast<float>(i * 16), 0.0f, chatColor);
    }

    // Weryfikacja: dokladnie 100 napisow w partii
    CHECK(batcher.GetBatchedStringCount() == 100);
    CHECK(batcher.GetBatchedGlyphCount() > 0);

    const size_t expectedVertices = batcher.GetBatchedGlyphCount() * 4;
    const size_t expectedIndices = batcher.GetBatchedGlyphCount() * 6;
    CHECK(batcher.GetVertexCount() == expectedVertices);
    CHECK(batcher.GetIndexCount() == expectedIndices);

    // Inicjalizacja bufora pierscieniowego w trybie headless RAM emulation
    DynamicGeometryRingBuffer ringBuffer(2 * 1024 * 1024, 1024 * 1024);
    REQUIRE(ringBuffer.Initialize(nullptr));

    // Flush - transfer 100 napisow do GPU/ringBuffera
    const bool flushOk = batcher.Flush(&ringBuffer, nullptr);
    CHECK(flushOk == true);

    // Kluczowa weryfikacja: dokladnie 1 Draw Call dla wszystkich 100 napisow
    CHECK(batcher.GetDrawCallsCount() == 1);
    CHECK(batcher.GetBatchedStringCount() == 100);
    CHECK(batcher.GetVertexCount() == 0); // Partia wyczyszczona po wyrysowaniu
    CHECK(batcher.GetIndexCount() == 0);

    // Kolejny flush na pustej partii zwraca 0 draw calls
    batcher.ResetFrame();
    CHECK(batcher.GetBatchedStringCount() == 0);
    CHECK(batcher.GetBatchedGlyphCount() == 0);
    const bool emptyFlush = batcher.Flush(&ringBuffer, nullptr);
    CHECK(emptyFlush == false);
    CHECK(batcher.GetDrawCallsCount() == 0);
}

TEST_CASE("MSDF - Test 3: Weryfikacja alokacji w DynamicGeometryRingBuffer (offsety, wierzcholki, indeksy)")
{
    MSDFFontAtlas atlas;
    atlas.LoadDefaultAsciiMetrics(512.0f, 512.0f, 16.0f);

    DynamicGeometryRingBuffer ringBuffer(64 * 1024, 16 * 1024);
    REQUIRE(ringBuffer.Initialize(nullptr));
    CHECK(ringBuffer.GetVertexOffset() == 0);
    CHECK(ringBuffer.GetIndexOffset() == 0);

    MSDFTextBatcher batcher(&atlas);

    // Dodanie krotkiego napisu testowego "SDF" (3 glify drukowalne)
    constexpr uint32_t textColor = 0xFF00FF00;
    constexpr uint32_t outlineColor = 0xFF112233;
    constexpr float outlineWidth = 0.25f;
    constexpr float pxRange = 4.0f;
    constexpr float testZ = 0.75f;

    batcher.AddText("SDF", 50.0f, 100.0f, testZ, textColor, outlineColor, outlineWidth, pxRange);

    CHECK(batcher.GetBatchedStringCount() == 1);
    CHECK(batcher.GetBatchedGlyphCount() == 3);
    CHECK(batcher.GetVertexCount() == 12); // 3 * 4
    CHECK(batcher.GetIndexCount() == 18);  // 3 * 6

    // Flush
    const bool flushOk = batcher.Flush(&ringBuffer, nullptr);
    CHECK(flushOk == true);
    CHECK(batcher.GetDrawCallsCount() == 1);

    // Weryfikacja offsetow w DynamicGeometryRingBuffer
    const uint32_t expectedVertexBytes = 12 * static_cast<uint32_t>(sizeof(MSDFVertex));
    const uint32_t expectedIndexBytes = 18 * static_cast<uint32_t>(sizeof(uint16_t));
    CHECK(ringBuffer.GetVertexOffset() == expectedVertexBytes);
    CHECK(ringBuffer.GetIndexOffset() == expectedIndexBytes);
    // 2 trafienia NOOVERWRITE: 1 alokacja w buforze wierzcholkow + 1 alokacja w buforze indeksow
    CHECK(ringBuffer.GetNoOverwriteHits() == 2);

    // Weryfikacja bezposrednich danych w zaalokowanej pamieci
    const auto& lastFlush = batcher.GetLastFlushInfo();
    REQUIRE(lastFlush.success == true);
    REQUIRE(lastFlush.vertexCount == 12);
    REQUIRE(lastFlush.indexCount == 18);
    REQUIRE(lastFlush.vertexAlloc.pData != nullptr);
    REQUIRE(lastFlush.indexAlloc.pData != nullptr);

    // Sprawdzenie wierzcholkow
    const auto* pVertices = static_cast<const MSDFVertex*>(lastFlush.vertexAlloc.pData);
    for (size_t i = 0; i < 12; ++i)
    {
        CHECK(pVertices[i].z == testZ);
        CHECK(pVertices[i].color == textColor);
        CHECK(pVertices[i].outlineColor == outlineColor);
        CHECK(pVertices[i].outlineWidth == outlineWidth);
        CHECK(pVertices[i].pxRange == pxRange);
    }

    // Sprawdzenie ukladu indeksow dla 3 quadow: (0,1,2, 2,3,0), (4,5,6, 6,7,4), (8,9,10, 10,11,8)
    const auto* pIndices = static_cast<const uint16_t*>(lastFlush.indexAlloc.pData);
    const uint16_t expectedIndexPattern[18] = {
        0, 1, 2,  2, 3, 0,
        4, 5, 6,  6, 7, 4,
        8, 9, 10, 10, 11, 8
    };

    for (size_t i = 0; i < 18; ++i)
    {
        CHECK(pIndices[i] == expectedIndexPattern[i]);
    }

    // Druga alokacja (kolejna partia) w tym samym buforze - weryfikacja przesuniecia offsetu i NOOVERWRITE
    batcher.AddText("UI", 0.0f, 0.0f);
    CHECK(batcher.Flush(&ringBuffer, nullptr));
    // Kolejne 2 trafienia (razem 4: 2 VB + 2 IB)
    CHECK(ringBuffer.GetNoOverwriteHits() == 4);
    CHECK(ringBuffer.GetVertexOffset() == expectedVertexBytes + (8 * sizeof(MSDFVertex)));
    CHECK(ringBuffer.GetIndexOffset() == expectedIndexBytes + (12 * sizeof(uint16_t)));
}

TEST_CASE("MSDF - Test 4: Zgodnosc matematyczna filtru mediany MSDF")
{
    // Podstawowa formula: median(r, g, b) = max(min(r, g), min(max(r, g), b)) - 0.5f

    // 1. Neutralne punkty pola odleglosci
    CHECK(ComputeMSDFMedian(0.5f, 0.5f, 0.5f) == doctest::Approx(0.0f));
    CHECK(ComputeMSDFMedian(1.0f, 1.0f, 1.0f) == doctest::Approx(0.5f));
    CHECK(ComputeMSDFMedian(0.0f, 0.0f, 0.0f) == doctest::Approx(-0.5f));

    // 2. Znane wektory odleglosci kanalu
    // Sortujac {0.2, 0.5, 0.8} -> mediana 0.5 -> signed dist 0.0
    CHECK(ComputeMSDFMedian(0.2f, 0.5f, 0.8f) == doctest::Approx(0.0f));

    // Sortujac {0.1, 0.4, 0.9} -> mediana 0.4 -> signed dist -0.1
    CHECK(ComputeMSDFMedian(0.9f, 0.1f, 0.4f) == doctest::Approx(-0.1f));

    // Sortujac {0.3, 0.7, 0.7} -> mediana 0.7 -> signed dist 0.2
    CHECK(ComputeMSDFMedian(0.3f, 0.7f, 0.7f) == doctest::Approx(0.2f));

    // 3. Symetria wzgledem permutacji (niezmiennik kolejnosci kanalow)
    const float r = 0.15f;
    const float g = 0.62f;
    const float b = 0.88f;
    const float expectedMedian = 0.62f - 0.5f;

    CHECK(ComputeMSDFMedian(r, g, b) == doctest::Approx(expectedMedian));
    CHECK(ComputeMSDFMedian(r, b, g) == doctest::Approx(expectedMedian));
    CHECK(ComputeMSDFMedian(g, r, b) == doctest::Approx(expectedMedian));
    CHECK(ComputeMSDFMedian(g, b, r) == doctest::Approx(expectedMedian));
    CHECK(ComputeMSDFMedian(b, r, g) == doctest::Approx(expectedMedian));
    CHECK(ComputeMSDFMedian(b, g, r) == doctest::Approx(expectedMedian));

    // 4. Test progu krawedzi (Edge Threshold):
    // Na zewnatrz ksztaltu glifu (odleglosc < 0.0f)
    CHECK(ComputeMSDFMedian(0.48f, 0.49f, 0.47f) < 0.0f);
    // Wewnatrz ksztaltu glifu (odleglosc > 0.0f)
    CHECK(ComputeMSDFMedian(0.51f, 0.52f, 0.53f) > 0.0f);

    // 5. Zgodnosc z ComputeRawMedian
    CHECK(ComputeRawMedian(0.2f, 0.5f, 0.8f) == doctest::Approx(0.5f));
    CHECK(ComputeRawMedian(r, g, b) - 0.5f == doctest::Approx(ComputeMSDFMedian(r, g, b)));
}
