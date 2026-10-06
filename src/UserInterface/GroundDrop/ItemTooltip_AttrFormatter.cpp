#include "../StdAfx.h"
#include "IFastItemTooltipCache.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

#include <unordered_map>
#include <string>
#include <format>
#include <expected>
#include <mutex>
#include <vector>

namespace UserInterface::GroundDrop
{
    /**
     * @brief Zdarzenie rozglaszane, gdy sformatowano i zbuforowano nowy tooltip przedmiotu.
     */
    struct TooltipFormattedEvent : public Core::IEvent
    {
    private:
        TooltipFormattedEvent(EterBase::ItemVnum vnum, const FormattedTooltipData& data)
            : itemVnum(vnum), tooltipData(data)
        {}

    public:
        EterBase::ItemVnum itemVnum;
        FormattedTooltipData tooltipData;

        static std::expected<TooltipFormattedEvent, std::string_view> Create(EterBase::ItemVnum vnum, const FormattedTooltipData& data)
        {
            if (vnum.value() == 0)
                return std::unexpected("Invalid item VNUM");
            if (data.title.empty())
                return std::unexpected("Tooltip title cannot be empty");

            return TooltipFormattedEvent(vnum, data);
        }
    };

    /**
     * @brief Klasa implementujaca pamiec podreczna (cache) wygenerowanych opisow przedmiotow (tooltipow) dla O(1) dostepu.
     */
    class FastItemTooltipCache : public IFastItemTooltipCache
    {
    public:
        void CacheTooltip(EterBase::ItemVnum vnum, const FormattedTooltipData& data) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cache[vnum.value()] = data;

            EterBase::ModernLogger::Info("Cached tooltip for ItemVnum: {}", vnum.value());

            auto eventResult = TooltipFormattedEvent::Create(vnum, data);
            if (eventResult)
            {
                Core::EventBus::GetInstance().Publish(*eventResult);
            }
            else
            {
                EterBase::ModernLogger::Error("Failed to create TooltipFormattedEvent: {}", eventResult.error());
            }
        }

        std::optional<FormattedTooltipData> GetTooltip(EterBase::ItemVnum vnum) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_cache.find(vnum.value());
            if (it != m_cache.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        void Invalidate(EterBase::ItemVnum vnum) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_cache.erase(vnum.value()) > 0)
            {
                EterBase::ModernLogger::Debug("Invalidated tooltip cache for ItemVnum: {}", vnum.value());
            }
        }

        void ClearAll() override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cache.clear();
            EterBase::ModernLogger::Info("Cleared all fast item tooltip caches");
        }

    private:
        mutable std::mutex m_mutex;
        std::unordered_map<uint32_t, FormattedTooltipData> m_cache;
    };

    /**
     * @brief Formatter dla atrybutow przedmiotu z uwzglednieniem bonow.
     */
    class ItemTooltipAttrFormatter
    {
    public:
        struct ItemAttribute
        {
            enum class Type {
                None,
                StrongAgainstHalfHumans,
                AverageDamage
            };
            
            Type type;
            int16_t value;
        };

        static std::expected<FormattedTooltipData, std::string_view> FormatAttributes(
            const std::string& baseName,
            const std::vector<ItemAttribute>& attributes,
            int32_t sellPrice)
        {
            if (baseName.empty())
                return std::unexpected("Base name cannot be empty");

            FormattedTooltipData data;
            data.title = baseName;
            data.titleColor = 0xFFFFFFAA; // Default light yellow color for title
            data.sellPrice = sellPrice;

            std::string description = "";

            for (const auto& attr : attributes)
            {
                if (!description.empty())
                    description += "\n";

                switch (attr.type)
                {
                    case ItemAttribute::Type::StrongAgainstHalfHumans:
                        description += std::format("Silny przeciwko Ludziom: +{}%", attr.value);
                        break;
                    case ItemAttribute::Type::AverageDamage:
                        description += std::format("Srednie Obrazenia: {}%", attr.value);
                        break;
                    default:
                        // Ignoruj nieznane atrybuty w tym formatterze
                        break;
                }
            }
            
            data.description = description;

            EterBase::ModernLogger::Debug("Formatted attributes for item: {}", baseName);

            return data;
        }
    };
}
