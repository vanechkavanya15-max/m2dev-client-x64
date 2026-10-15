#include "MSDFFontRenderer.h"

#include <cmath>

namespace Client::Graphics
{

char32_t DecodeNextUtf8Codepoint(std::string_view text, size_t& offset) noexcept
{
    if (offset >= text.size())
    {
        return 0;
    }

    const auto b0 = static_cast<uint8_t>(text[offset++]);
    if (b0 < 0x80)
    {
        return static_cast<char32_t>(b0);
    }
    else if ((b0 & 0xE0) == 0xC0)
    {
        if (offset < text.size())
        {
            const auto b1 = static_cast<uint8_t>(text[offset++]);
            return (static_cast<char32_t>(b0 & 0x1F) << 6) |
                   static_cast<char32_t>(b1 & 0x3F);
        }
    }
    else if ((b0 & 0xF0) == 0xE0)
    {
        if (offset + 1 < text.size())
        {
            const auto b1 = static_cast<uint8_t>(text[offset++]);
            const auto b2 = static_cast<uint8_t>(text[offset++]);
            return (static_cast<char32_t>(b0 & 0x0F) << 12) |
                   (static_cast<char32_t>(b1 & 0x3F) << 6) |
                   static_cast<char32_t>(b2 & 0x3F);
        }
    }
    else if ((b0 & 0xF8) == 0xF0)
    {
        if (offset + 2 < text.size())
        {
            const auto b1 = static_cast<uint8_t>(text[offset++]);
            const auto b2 = static_cast<uint8_t>(text[offset++]);
            const auto b3 = static_cast<uint8_t>(text[offset++]);
            return (static_cast<char32_t>(b0 & 0x07) << 18) |
                   (static_cast<char32_t>(b1 & 0x3F) << 12) |
                   (static_cast<char32_t>(b2 & 0x3F) << 6) |
                   static_cast<char32_t>(b3 & 0x3F);
        }
    }

    return static_cast<char32_t>(b0);
}

bool MSDFFontAtlas::RegisterGlyph(char32_t codepoint, const GlyphMetric& metric) noexcept
{
    m_glyphs[codepoint] = metric;
    return true;
}

const GlyphMetric* MSDFFontAtlas::GetGlyph(char32_t codepoint) const noexcept
{
    const auto it = m_glyphs.find(codepoint);
    if (it != m_glyphs.end())
    {
        return &it->second;
    }

    if (m_fallbackCodepoint != 0 && codepoint != m_fallbackCodepoint)
    {
        const auto fallbackIt = m_glyphs.find(m_fallbackCodepoint);
        if (fallbackIt != m_glyphs.end())
        {
            return &fallbackIt->second;
        }
    }

    return nullptr;
}

bool MSDFFontAtlas::HasGlyph(char32_t codepoint) const noexcept
{
    return m_glyphs.contains(codepoint);
}

TextDimension MSDFFontAtlas::CalculateTextSize(std::string_view text, float scale) const noexcept
{
    if (text.empty())
    {
        return TextDimension{.width = 0.0f, .height = 0.0f};
    }

    float maxWidth = 0.0f;
    float currentLineWidth = 0.0f;
    size_t lineCount = 1;

    size_t offset = 0;
    while (offset < text.size())
    {
        const char32_t cp = DecodeNextUtf8Codepoint(text, offset);
        if (cp == 0)
        {
            break;
        }

        if (cp == U'\r')
        {
            continue;
        }

        if (cp == U'\n')
        {
            maxWidth = std::max(maxWidth, currentLineWidth);
            currentLineWidth = 0.0f;
            ++lineCount;
            continue;
        }

        const auto* glyph = GetGlyph(cp);
        if (glyph)
        {
            currentLineWidth += glyph->advanceX * scale;
        }
    }

    maxWidth = std::max(maxWidth, currentLineWidth);
    const float totalHeight = static_cast<float>(lineCount) * m_lineHeight * scale;

    return TextDimension{
        .width = maxWidth,
        .height = totalHeight
    };
}

void MSDFFontAtlas::LoadDefaultAsciiMetrics(float atlasWidth, float atlasHeight, float fontSize) noexcept
{
    m_glyphs.clear();
    m_textureWidth = atlasWidth;
    m_textureHeight = atlasHeight;
    m_lineHeight = fontSize * 1.25f;
    m_baseLine = fontSize * 0.85f;
    m_defaultPxRange = 4.0f;
    m_fallbackCodepoint = U'?';

    constexpr uint32_t kGridCols = 16;
    const float cellWidth = atlasWidth / static_cast<float>(kGridCols);
    const float cellHeight = atlasHeight / static_cast<float>(kGridCols);

    // Spacja (ASCII 32)
    m_glyphs[U' '] = GlyphMetric{
        .u0 = 0.0f,
        .v0 = 0.0f,
        .u1 = 0.0f,
        .v1 = 0.0f,
        .width = 0.0f,
        .height = 0.0f,
        .offsetX = 0.0f,
        .offsetY = 0.0f,
        .advanceX = fontSize * 0.45f
    };

    // Tabulator
    m_glyphs[U'\t'] = GlyphMetric{
        .u0 = 0.0f,
        .v0 = 0.0f,
        .u1 = 0.0f,
        .v1 = 0.0f,
        .width = 0.0f,
        .height = 0.0f,
        .offsetX = 0.0f,
        .offsetY = 0.0f,
        .advanceX = fontSize * 0.45f * 4.0f
    };

    // Znaki drukowalne ASCII od 33 (!) do 126 (~)
    for (char32_t c = 33; c <= 126; ++c)
    {
        const uint32_t idx = static_cast<uint32_t>(c - 32);
        const uint32_t row = idx / kGridCols;
        const uint32_t col = idx % kGridCols;

        const float u0 = (static_cast<float>(col) * cellWidth) / atlasWidth;
        const float v0 = (static_cast<float>(row) * cellHeight) / atlasHeight;
        const float u1 = (static_cast<float>(col + 1) * cellWidth) / atlasWidth;
        const float v1 = (static_cast<float>(row + 1) * cellHeight) / atlasHeight;

        m_glyphs[c] = GlyphMetric{
            .u0 = u0,
            .v0 = v0,
            .u1 = u1,
            .v1 = v1,
            .width = fontSize * 0.65f,
            .height = fontSize,
            .offsetX = 0.0f,
            .offsetY = 0.0f,
            .advanceX = fontSize * 0.70f
        };
    }
}

void MSDFFontAtlas::Clear() noexcept
{
    m_glyphs.clear();
}

} // namespace Client::Graphics
