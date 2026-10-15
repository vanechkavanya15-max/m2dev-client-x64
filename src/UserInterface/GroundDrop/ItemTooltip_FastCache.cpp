#include "../StdAfx.h"
#include "IFastItemTooltipCache.h"
#include "EterBase/LogModern.h"
#include "Core/EventBus.h"
#include <unordered_map>
#include <mutex>
#include <memory>

namespace UserInterface::GroundDrop
{
    struct TooltipCachedEvent : public Core::IEvent
    {
        EterBase::ItemVnum vnum;
        
        explicit TooltipCachedEvent(EterBase::ItemVnum vnum) : vnum(vnum) {}
    };

    struct TooltipInvalidatedEvent : public Core::IEvent
    {
        EterBase::ItemVnum vnum;
        
        explicit TooltipInvalidatedEvent(EterBase::ItemVnum vnum) : vnum(vnum) {}
    };

    struct TooltipCacheClearedEvent : public Core::IEvent
    {
        TooltipCacheClearedEvent() = default;
    };

    class ItemTooltipFastCache : public IFastItemTooltipCache
    {
    public:
        ItemTooltipFastCache() = default;
        ~ItemTooltipFastCache() override = default;

        void CacheTooltip(EterBase::ItemVnum vnum, const FormattedTooltipData& data) override
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cache[vnum] = data;
            lock.unlock();
            
            EterBase::ModernLogger::Debug("Cached tooltip for item {}", vnum.value());
            Core::EventBus::GetInstance().Publish(TooltipCachedEvent{vnum});
        }

        std::optional<FormattedTooltipData> GetTooltip(EterBase::ItemVnum vnum) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_cache.find(vnum);
            if (it != m_cache.end())
            {
                EterBase::ModernLogger::Trace("Tooltip cache hit for item {}", vnum.value());
                return it->second;
            }
            
            EterBase::ModernLogger::Trace("Tooltip cache miss for item {}", vnum.value());
            return std::nullopt;
        }

        void Invalidate(EterBase::ItemVnum vnum) override
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            auto it = m_cache.find(vnum);
            if (it != m_cache.end())
            {
                m_cache.erase(it);
                lock.unlock();
                EterBase::ModernLogger::Debug("Invalidated tooltip cache for item {}", vnum.value());
                Core::EventBus::GetInstance().Publish(TooltipInvalidatedEvent{vnum});
            }
        }

        void ClearAll() override
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cache.clear();
            lock.unlock();
            
            EterBase::ModernLogger::Info("Cleared all tooltip caches");
            Core::EventBus::GetInstance().Publish(TooltipCacheClearedEvent{});
        }

    private:
        mutable std::mutex m_mutex;
        std::unordered_map<EterBase::ItemVnum, FormattedTooltipData> m_cache;
    };

} // namespace UserInterface::GroundDrop
