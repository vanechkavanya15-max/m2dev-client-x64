#include "../StdAfx.h"
#include "IFastItemTooltipCache.h"
#include "../../GameLib/ItemManager.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../Core/EventBus.h"
#include <unordered_map>
#include <mutex>
#include <format>

namespace UserInterface::GroundDrop
{
    class FastItemTooltipCacheImpl final : public IFastItemTooltipCache
    {
    public:
        void CacheTooltip(EterBase::ItemVnum vnum, const FormattedTooltipData& data) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cache[vnum.value()] = data;
            
            EterBase::ModernLogger::Debug("Cached tooltip for item {}", vnum.value());
            Core::EventBus::GetInstance().Publish(Core::ItemTooltipCachedEvent{vnum});
        }

        std::optional<FormattedTooltipData> GetTooltip(EterBase::ItemVnum vnum) const override
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_cache.find(vnum.value());
                if (it != m_cache.end())
                {
                    return it->second;
                }
            }

            // Not found in cache, build it now
            auto builtTooltip = BuildTooltipDesc(vnum);
            if (builtTooltip)
            {
                const_cast<FastItemTooltipCacheImpl*>(this)->CacheTooltip(vnum, builtTooltip.value());
                return builtTooltip.value();
            }

            return std::nullopt;
        }

        void Invalidate(EterBase::ItemVnum vnum) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_cache.erase(vnum.value()) > 0)
            {
                EterBase::ModernLogger::Debug("Invalidated tooltip cache for item {}", vnum.value());
            }
        }

        void ClearAll() override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cache.clear();
            EterBase::ModernLogger::Info("Cleared all item tooltip caches.");
        }

    private:
        std::expected<FormattedTooltipData, EterBase::InventoryError> BuildTooltipDesc(EterBase::ItemVnum vnum) const
        {
            CItemData* pItemData = nullptr;
            if (!CItemManager::Instance().GetItemDataPointer(vnum.value(), &pItemData) || !pItemData)
            {
                EterBase::ModernLogger::Error("Failed to build tooltip: Invalid item vnum {}", vnum.value());
                return std::unexpected(EterBase::InventoryError::InvalidVnum);
            }

            FormattedTooltipData data;
            
            // Name
            const char* name = pItemData->GetName();
            if (name)
            {
                data.title = name;
            }
            else
            {
                data.title = std::format("Item {}", vnum.value());
            }

            // Description
            const char* desc = pItemData->GetDescription();
            if (desc)
            {
                data.description = desc;
            }

            // Append Summary to description if available
            const char* summary = pItemData->GetSummary();
            if (summary && summary[0] != '\0')
            {
                if (!data.description.empty())
                {
                    data.description += "\n";
                }
                data.description += summary;
            }

            // Sell Price
            data.sellPrice = pItemData->GetISellItemPrice();

            // Color formatting logic based on item type
            // E.g., red for weapons, green for armor, etc.
            // Using standard color constants or values for UI
            switch (pItemData->GetType())
            {
                case CItemData::ITEM_TYPE_WEAPON:
                    data.titleColor = 0xFFFF4A4A; // Reddish
                    break;
                case CItemData::ITEM_TYPE_ARMOR:
                    data.titleColor = 0xFF4AFF4A; // Greenish
                    break;
                case CItemData::ITEM_TYPE_USE:
                    data.titleColor = 0xFF4A4AFF; // Blueish
                    break;
                default:
                    data.titleColor = 0xFFFFFFFF; // White
                    break;
            }

            EterBase::ModernLogger::Debug("Successfully built tooltip description for item {}", vnum.value());
            return data;
        }

        mutable std::mutex m_mutex;
        std::unordered_map<uint32_t, FormattedTooltipData> m_cache;
    };

    std::unique_ptr<IFastItemTooltipCache> CreateFastItemTooltipCache()
    {
        return std::make_unique<FastItemTooltipCacheImpl>();
    }
}
