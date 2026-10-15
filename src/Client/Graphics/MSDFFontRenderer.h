#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <unordered_map>
#include <algorithm>

namespace Client::Graphics
{

/**
 * @brief Struktura opisujaca metryki pojedynczego glifu w atlasie czcionki MSDF.
 */
struct GlyphMetric
{
    float u0{0.0f};       ///< Lewa wspolrzedna tekstury UV (0.0 .. 1.0)
    float v0{0.0f};       ///< Gorna wspolrzedna tekstury UV (0.0 .. 1.0)
    float u1{0.0f};       ///< Prawa wspolrzedna tekstury UV (0.0 .. 1.0)
    float v1{0.0f};       ///< Dolna wspolrzedna tekstury UV (0.0 .. 1.0)
    float width{0.0f};    ///< Szerokosc geometryczna glifu w pikselach ekranu
    float height{0.0f};   ///< Wysokosc geometryczna glifu w pikselach ekranu
    float offsetX{0.0f};  ///< Przesuniecie poziome wzgledem biezacej pozycji kursora
    float offsetY{0.0f};  ///< Przesuniecie pionowe wzgledem linii bazowej
    float advanceX{0.0f}; ///< Krok poziomy kursora po narysowaniu tego glifu
};

/**
 * @brief Struktura wierzcholka dla renderera tekstu MSDF (Multi-channel Signed Distance Field).
 * Rozmiar dokladnie 36 bajtow, zoptymalizowany pod Direct3D 9 Vertex Declaration i dynamiczny ring buffer.
 */
struct MSDFVertex
{
    float x{0.0f};              ///< Pozycja X w przestrzeni ekranowej/swiata
    float y{0.0f};              ///< Pozycja Y w przestrzeni ekranowej/swiata
    float z{0.0f};              ///< Pozycja Z (glebokosc)
    float u{0.0f};              ///< Wspolrzedna tekstury U w atlasie MSDF
    float v{0.0f};              ///< Wspolrzedna tekstury V w atlasie MSDF
    uint32_t color{0xFFFFFFFF}; ///< Kolor glifu (RGBA/ARGB, domyslnie pelna biel)
    uint32_t outlineColor{0x00000000}; ///< Kolor obrysu (domyslnie przezroczysty)
    float outlineWidth{0.0f};   ///< Grubosc obrysu w jednostkach odleglosci (0.0 = brak obrysu)
    float pxRange{4.0f};        ///< Zakres pikseli MSDF (pixel range / odleglosc pola)
};

static_assert(sizeof(MSDFVertex) == 36, "MSDFVertex rozmiar musi wynosic dokladnie 36 bajtow");

/**
 * @brief Struktura opisujaca wymiary zmierzonego tekstu.
 */
struct TextDimension
{
    float width{0.0f};  ///< Maksymalna szerokosc tekstu w pikselach
    float height{0.0f}; ///< Calkowita wysokosc tekstu w pikselach (uwzglednia wiele linii)
};

/**
 * @brief Matematyka filtru medianowego dla probek wielokanalowego pola odleglosci MSDF (R, G, B).
 * Wzor: median(r, g, b) = max(min(r, g), min(max(r, g), b)) - 0.5f.
 * Wartosc 0.0 oznacza idealna krawedz znaku. Wartosci dodatnie leza wewnatrz, ujemne na zewnatrz.
 * @param r Kanal czerwony (dystans dla pierwszej grupy krawedzi).
 * @param g Kanal zielony (dystans dla drugiej grupy krawedzi).
 * @param b Kanal niebieski (dystans dla trzeciej grupy krawedzi).
 * @return Wycentrowana wartosc signed distance (odleglosc podpisana).
 */
[[nodiscard]] constexpr float ComputeMSDFMedian(float r, float g, float b) noexcept
{
    return std::max(std::min(r, g), std::min(std::max(r, g), b)) - 0.5f;
}

/**
 * @brief Surowa mediana z 3 wartosci bez odjecia 0.5f.
 */
[[nodiscard]] constexpr float ComputeRawMedian(float r, float g, float b) noexcept
{
    return std::max(std::min(r, g), std::min(std::max(r, g), b));
}

/**
 * @brief Dekoduje kolejny punkt kodowy UTF-8 ze strumienia string_view.
 * @param text Widok ciagu znakow UTF-8.
 * @param offset Biezacy indeks w tekscie (zwiekszany o liczbe zuzytych bajtow).
 * @return Punkt kodowy Unicode (char32_t).
 */
char32_t DecodeNextUtf8Codepoint(std::string_view text, size_t& offset) noexcept;

/**
 * @class MSDFFontAtlas
 * @brief Zarzadza metrykami glifow czcionki MSDF (ASCII oraz znaki rozszerzone Unicode).
 */
class MSDFFontAtlas
{
public:
    static constexpr char32_t kDefaultFallbackCodepoint = U'?';

    MSDFFontAtlas() noexcept = default;
    ~MSDFFontAtlas() = default;

    MSDFFontAtlas(const MSDFFontAtlas&) = default;
    MSDFFontAtlas& operator=(const MSDFFontAtlas&) = default;
    MSDFFontAtlas(MSDFFontAtlas&&) noexcept = default;
    MSDFFontAtlas& operator=(MSDFFontAtlas&&) noexcept = default;

    /**
     * @brief Rejestruje metryke pojedynczego punktu kodowego.
     */
    bool RegisterGlyph(char32_t codepoint, const GlyphMetric& metric) noexcept;

    /**
     * @brief Pobiera metryke wskazanego punktu kodowego.
     * Jesli punkt kodowy nie istnieje, probuje zwrocic glif zastepczy (fallback).
     */
    [[nodiscard]] const GlyphMetric* GetGlyph(char32_t codepoint) const noexcept;

    /**
     * @brief Sprawdza czy dany glif jest zarejestrowany bezposrednio w atlasie.
     */
    [[nodiscard]] bool HasGlyph(char32_t codepoint) const noexcept;

    /**
     * @brief Oblicza geometryczny rozmiar tekstu (obsluguje \n i UTF-8).
     */
    [[nodiscard]] TextDimension CalculateTextSize(std::string_view text, float scale = 1.0f) const noexcept;

    /**
     * @brief Laduje standardowa siatke metryk dla calego zakresu znakow drukowalnych ASCII (32..126).
     */
    void LoadDefaultAsciiMetrics(float atlasWidth = 512.0f, float atlasHeight = 512.0f, float fontSize = 16.0f) noexcept;

    /**
     * @brief Czysci wszystkie zarejestrowane metryki glifow.
     */
    void Clear() noexcept;

    // Gettery i settery parametrow czcionki
    [[nodiscard]] size_t GetGlyphCount() const noexcept { return m_glyphs.size(); }
    [[nodiscard]] float GetLineHeight() const noexcept { return m_lineHeight; }
    [[nodiscard]] float GetBaseLine() const noexcept { return m_baseLine; }
    [[nodiscard]] float GetDefaultPxRange() const noexcept { return m_defaultPxRange; }
    [[nodiscard]] float GetTextureWidth() const noexcept { return m_textureWidth; }
    [[nodiscard]] float GetTextureHeight() const noexcept { return m_textureHeight; }
    [[nodiscard]] char32_t GetFallbackCodepoint() const noexcept { return m_fallbackCodepoint; }

    void SetLineHeight(float lineHeight) noexcept { m_lineHeight = lineHeight; }
    void SetBaseLine(float baseLine) noexcept { m_baseLine = baseLine; }
    void SetDefaultPxRange(float pxRange) noexcept { m_defaultPxRange = pxRange; }
    void SetTextureDimensions(float width, float height) noexcept
    {
        m_textureWidth = width;
        m_textureHeight = height;
    }
    void SetFallbackCodepoint(char32_t fallback) noexcept { m_fallbackCodepoint = fallback; }

private:
    std::unordered_map<char32_t, GlyphMetric> m_glyphs;
    float m_textureWidth{512.0f};
    float m_textureHeight{512.0f};
    float m_lineHeight{20.0f};
    float m_baseLine{16.0f};
    float m_defaultPxRange{4.0f};
    char32_t m_fallbackCodepoint{kDefaultFallbackCodepoint};
};

} // namespace Client::Graphics
