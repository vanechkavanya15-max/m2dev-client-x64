#include "../StdAfx.h"
#include "ITextTailService.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "UserInterface/Core/EventBus.h"
#include "EterBase/StrongTypes.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

namespace UserInterface::TextTail
{
    // Caching for generated atlas text glyphs to prevent continuous reallocations
    struct AtlasGlyph
    {
        uint32_t charCode;
        float u, v, width, height;
    };

    struct CachedTail
    {
        uint32_t virtualId;
        std::string text;
        uint32_t color;
        float offsetY;
        bool active;
    };

    struct TextTailCacheUpdateEvent : public UserInterface::Core::IEvent
    {
        uint32_t virtualId;
        bool registered;
        TextTailCacheUpdateEvent(uint32_t id, bool reg) : virtualId(id), registered(reg) {}
    };

    class TextTailFontCache : public ITextTailService
    {
    public:
        TextTailFontCache()
        {
            EterBase::ModernLogger::Info("TextTailFontCache initialized with preallocated pool and atlas glyph cache");
            m_tailPool.resize(MAX_TAILS);
            for (auto& tail : m_tailPool) {
                tail.active = false;
            }
            m_glyphCache.reserve(MAX_GLYPHS);
        }

        ~TextTailFontCache() override = default;

        EterBase::PacketResult<void> RegisterActorTail(const TextTailCreateData& data) override
        {
            if (data.vid.value() == 0) {
                EterBase::ModernLogger::Error("Failed to register actor tail: Invalid entity ID");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            CachedTail* freeTail = FindFreeTail();
            if (!freeTail) {
                EterBase::ModernLogger::Error("Failed to register actor tail for VID {}: Pool exhausted", data.vid.value());
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            freeTail->virtualId = data.vid.value();
            freeTail->text = data.text;
            freeTail->color = data.color;
            freeTail->offsetY = data.offsetY;
            freeTail->active = true;

            m_tailLookup[data.vid.value()] = freeTail;

            CacheGlyphsForText(data.text);
            
            UserInterface::Core::EventBus::GetInstance().Publish(TextTailCacheUpdateEvent(data.vid.value(), true));

            EterBase::ModernLogger::Info("Actor tail registered for VID {}", data.vid.value());
            return {};
        }

        EterBase::PacketResult<void> RegisterItemTail(uint32_t virtualId, std::string_view name) override
        {
            if (virtualId == 0) {
                EterBase::ModernLogger::Error("Failed to register item tail: Invalid virtual ID");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            CachedTail* freeTail = FindFreeTail();
            if (!freeTail) {
                EterBase::ModernLogger::Error("Failed to register item tail for ID {}: Pool exhausted", virtualId);
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            freeTail->virtualId = virtualId;
            freeTail->text = name;
            freeTail->color = 0xFFFFFFFF; // Default color
            freeTail->offsetY = 0.0f;
            freeTail->active = true;

            m_tailLookup[virtualId] = freeTail;

            CacheGlyphsForText(name);

            UserInterface::Core::EventBus::GetInstance().Publish(TextTailCacheUpdateEvent(virtualId, true));

            EterBase::ModernLogger::Info("Item tail registered for ID {}", virtualId);
            return {};
        }

        void RemoveTail(uint32_t virtualId) override
        {
            auto it = m_tailLookup.find(virtualId);
            if (it != m_tailLookup.end()) {
                it->second->active = false;
                m_tailLookup.erase(it);
                UserInterface::Core::EventBus::GetInstance().Publish(TextTailCacheUpdateEvent(virtualId, false));
                EterBase::ModernLogger::Info("Removed tail for ID {}", virtualId);
            } else {
                EterBase::ModernLogger::Warn("Attempted to remove non-existent tail for ID {}", virtualId);
            }
        }

        void UpdateScreenPositions(float viewMatrix[16], float projMatrix[16]) override
        {
            // Update matrices placeholder
            EterBase::ModernLogger::Trace("UpdateScreenPositions called");
        }

        void RenderBatch() override
        {
            // Placeholder for rendering batch logic that utilizes m_glyphCache.
            EterBase::ModernLogger::Trace("RenderBatch called utilizing cached atlas glyphs");
        }

        void ClearAll() override
        {
            for (auto& tail : m_tailPool) {
                if (tail.active) {
                     UserInterface::Core::EventBus::GetInstance().Publish(TextTailCacheUpdateEvent(tail.virtualId, false));
                }
                tail.active = false;
            }
            m_tailLookup.clear();
            m_glyphCache.clear();
            EterBase::ModernLogger::Info("Cleared all cached text tails and atlas glyphs");
        }

    private:
        static constexpr size_t MAX_TAILS = 1024;
        static constexpr size_t MAX_GLYPHS = 4096;

        std::vector<CachedTail> m_tailPool;
        std::unordered_map<uint32_t, CachedTail*> m_tailLookup;
        std::unordered_map<uint32_t, AtlasGlyph> m_glyphCache; // charCode -> AtlasGlyph

        CachedTail* FindFreeTail()
        {
            for (auto& tail : m_tailPool) {
                if (!tail.active) {
                    return &tail;
                }
            }
            return nullptr;
        }

        void CacheGlyphsForText(std::string_view text)
        {
            for (char c : text) {
                uint32_t charCode = static_cast<uint32_t>(c);
                if (m_glyphCache.find(charCode) == m_glyphCache.end()) {
                    // Generate pseudo-glyph data for caching
                    AtlasGlyph glyph;
                    glyph.charCode = charCode;
                    glyph.u = 0.0f;
                    glyph.v = 0.0f;
                    glyph.width = 10.0f;
                    glyph.height = 10.0f;
                    m_glyphCache[charCode] = glyph;
                }
            }
        }
    };

    std::unique_ptr<ITextTailService> CreateTextTailFontCache()
    {
        return std::make_unique<TextTailFontCache>();
    }
}
