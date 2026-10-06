#include "../StdAfx.h"
#include "ITargetHpBarService.h"
#include "../Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <algorithm>
#include <array>

// Define custom events in UserInterface::Core namespace to adhere to memory rules
namespace UserInterface::Core
{
    struct TargetHpBarVisibilityEvent : public IEvent
    {
        EterBase::EntityId vid;
        bool isVisible;
        
        TargetHpBarVisibilityEvent(EterBase::EntityId vid, bool visible)
            : vid(vid), isVisible(visible) {}
    };
    
    struct TargetHpBarUpdateEvent : public IEvent
    {
        EterBase::EntityId vid;
        uint32_t currentHp;
        uint32_t maxHp;
        
        TargetHpBarUpdateEvent(EterBase::EntityId vid, uint32_t current, uint32_t max)
            : vid(vid), currentHp(current), maxHp(max) {}
    };
}

namespace UserInterface::TextTail
{
    // Generic 2D Quad Vertex for UI geometry caching (decoupled from actual D3D rendering)
    struct QuadVertex {
        float x, y, z;
        uint32_t color;
        float u, v;
    };

    class TargetHpBarServiceCache : public ITargetHpBarService
    {
    private:
        struct HpBarCacheEntry {
            EterBase::EntityId vid;
            uint32_t currentHp{0};
            uint32_t maxHp{0};
            bool isActive{false};
            uint32_t lastUpdateTime{0};
            // Cache of quad geometry for the HP bar to prevent heap allocation per frame
            std::array<QuadVertex, 4> geometryBuffer{};
        };

        static constexpr size_t MAX_CACHE_SIZE = 128;

        std::optional<HpBarCacheEntry> m_currentTarget;
        std::vector<HpBarCacheEntry> m_miniMobCache;
        std::unordered_map<uint32_t, size_t> m_vidToIndex;
        uint32_t m_currentTime{0};

        // Internal methods using std::expected for robust error handling
        EterBase::Result<void, EterBase::EntityError> TryUpdateCacheEntry(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp)
        {
            if (!vid) {
                return std::unexpected(EterBase::EntityError::NotFound);
            }

            m_currentTime++; // Simple logical clock for LRU eviction

            // If dead, deactivate it
            if (currentHp == 0) {
                if (m_currentTarget && m_currentTarget->vid == vid) {
                    m_currentTarget->isActive = false;
                }
                auto it = m_vidToIndex.find(vid.value());
                if (it != m_vidToIndex.end()) {
                    m_miniMobCache[it->second].isActive = false;
                }
                return {};
            }

            if (m_currentTarget && m_currentTarget->vid == vid) {
                m_currentTarget->currentHp = currentHp;
                m_currentTarget->maxHp = maxHp;
                m_currentTarget->isActive = true;
                UpdateGeometry(m_currentTarget.value());
                return {};
            }

            auto it = m_vidToIndex.find(vid.value());
            if (it != m_vidToIndex.end()) {
                m_miniMobCache[it->second].currentHp = currentHp;
                m_miniMobCache[it->second].maxHp = maxHp;
                m_miniMobCache[it->second].isActive = true;
                m_miniMobCache[it->second].lastUpdateTime = m_currentTime;
                UpdateGeometry(m_miniMobCache[it->second]);
                return {};
            }

            if (m_miniMobCache.size() < MAX_CACHE_SIZE) {
                m_vidToIndex[vid.value()] = m_miniMobCache.size();
                HpBarCacheEntry entry{vid, currentHp, maxHp, true, m_currentTime, {}};
                UpdateGeometry(entry);
                m_miniMobCache.push_back(entry);
                return {};
            }

            // Eviction: first try to find inactive
            auto inactiveIt = std::find_if(m_miniMobCache.begin(), m_miniMobCache.end(), [](const HpBarCacheEntry& s) { return !s.isActive; });
            if (inactiveIt != m_miniMobCache.end()) {
                m_vidToIndex.erase(inactiveIt->vid.value());
                inactiveIt->vid = vid;
                inactiveIt->currentHp = currentHp;
                inactiveIt->maxHp = maxHp;
                inactiveIt->isActive = true;
                inactiveIt->lastUpdateTime = m_currentTime;
                UpdateGeometry(*inactiveIt);
                m_vidToIndex[vid.value()] = std::distance(m_miniMobCache.begin(), inactiveIt);
                return {};
            }

            // Fallback: LRU eviction (find oldest updated entry)
            auto oldestIt = std::min_element(m_miniMobCache.begin(), m_miniMobCache.end(),
                [](const HpBarCacheEntry& a, const HpBarCacheEntry& b) {
                    return a.lastUpdateTime < b.lastUpdateTime;
                });
                
            m_vidToIndex.erase(oldestIt->vid.value());
            oldestIt->vid = vid;
            oldestIt->currentHp = currentHp;
            oldestIt->maxHp = maxHp;
            oldestIt->isActive = true;
            oldestIt->lastUpdateTime = m_currentTime;
            UpdateGeometry(*oldestIt);
            m_vidToIndex[vid.value()] = std::distance(m_miniMobCache.begin(), oldestIt);

            return {};
        }

        void UpdateGeometry(HpBarCacheEntry& entry) {
            // Logical geometry update based on HP ratio
            float ratio = entry.maxHp > 0 ? static_cast<float>(entry.currentHp) / static_cast<float>(entry.maxHp) : 0.0f;
            entry.geometryBuffer[0] = {0.0f, 0.0f, 0.0f, 0xFF00FF00, 0.0f, 0.0f}; // Top-left
            entry.geometryBuffer[1] = {100.0f * ratio, 0.0f, 0.0f, 0xFF00FF00, ratio, 0.0f}; // Top-right
            entry.geometryBuffer[2] = {0.0f, 10.0f, 0.0f, 0xFF00FF00, 0.0f, 1.0f}; // Bottom-left
            entry.geometryBuffer[3] = {100.0f * ratio, 10.0f, 0.0f, 0xFF00FF00, ratio, 1.0f}; // Bottom-right
        }

    public:
        TargetHpBarServiceCache()
        {
            m_miniMobCache.reserve(MAX_CACHE_SIZE);
            EterBase::ModernLogger::Info("TargetHpBarServiceCache initialized with capacity {}", MAX_CACHE_SIZE);
        }

        ~TargetHpBarServiceCache() override = default;

        void ShowTargetBar(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) override
        {
            if (!vid)
            {
                EterBase::ModernLogger::Warn("ShowTargetBar called with invalid VID");
                return;
            }

            m_currentTime++;
            HpBarCacheEntry entry{vid, currentHp, maxHp, true, m_currentTime, {}};
            UpdateGeometry(entry);
            m_currentTarget = entry;
            
            EterBase::ModernLogger::Debug("ShowTargetBar: VID {}, HP {}/{}", vid.value(), currentHp, maxHp);
            
            Core::EventBus::GetInstance().Publish(Core::TargetHpBarVisibilityEvent(vid, true));
            Core::EventBus::GetInstance().Publish(Core::TargetHpBarUpdateEvent(vid, currentHp, maxHp));
        }

        void UpdateTargetHp(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) override
        {
            auto result = TryUpdateCacheEntry(vid, currentHp, maxHp);
            if (!result)
            {
                EterBase::ModernLogger::Warn("Failed to update HP for VID {}: {}", vid.value(), EterBase::ToString(result.error()));
                return;
            }
            Core::EventBus::GetInstance().Publish(Core::TargetHpBarUpdateEvent(vid, currentHp, maxHp));
        }

        void HideTargetBar() override
        {
            if (m_currentTarget)
            {
                EterBase::ModernLogger::Debug("HideTargetBar: VID {}", m_currentTarget->vid.value());
                Core::EventBus::GetInstance().Publish(Core::TargetHpBarVisibilityEvent(m_currentTarget->vid, false));
                m_currentTarget.reset();
            }
        }

        void RenderTargetBar() override
        {
            if (m_currentTarget && m_currentTarget->isActive)
            {
                // Here we would typically submit m_currentTarget->geometryBuffer to the renderer
                EterBase::ModernLogger::Trace("RenderTargetBar submitting geometry for VID {}", m_currentTarget->vid.value());
            }
        }

        void RenderMiniMobBars() override
        {
            for (auto& state : m_miniMobCache)
            {
                if (state.isActive)
                {
                   // Here we would typically submit state.geometryBuffer to the renderer
                   EterBase::ModernLogger::Trace("RenderMiniMobBars submitting geometry for VID {}", state.vid.value());
                }
            }
        }

        void Clear() override
        {
            EterBase::ModernLogger::Debug("TargetHpBarServiceCache cleared");
            HideTargetBar();
            
            for (auto& state : m_miniMobCache) {
                if (state.isActive) {
                    Core::EventBus::GetInstance().Publish(Core::TargetHpBarVisibilityEvent(state.vid, false));
                }
            }
            
            m_miniMobCache.clear();
            m_vidToIndex.clear();
        }
    };

    std::unique_ptr<ITargetHpBarService> CreateTargetHpBarService()
    {
        return std::make_unique<TargetHpBarServiceCache>();
    }
}
