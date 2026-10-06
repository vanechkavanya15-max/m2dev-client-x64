#include "../StdAfx.h"
#include "IFastItemTooltipCache.h"
#include "../Core/EventBus.h"
#include "../../EterBase/ModernLogger.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include <unordered_map>
#include <mutex>
#include <optional>
#include <format>
#include <vector>
#include <string>
#include <memory>

namespace UserInterface::GroundDrop
{
    /**
     * @brief Struktura reprezentujaca roznice w statystykach.
     */
    struct StatDifference
    {
        std::string statName;
        int32_t groundValue;
        int32_t equippedValue;
    };

    /**
     * @brief Zdarzenie rozgloszeniowe żądające statystyk ubranego przedmiotu w celu porównania.
     */
    struct ItemTooltipComparisonRequestEvent : public Core::IEvent
    {
        EterBase::ItemVnum groundItemVnum;
        std::vector<StatDifference>* outDifferences;

        ItemTooltipComparisonRequestEvent(EterBase::ItemVnum vnum, std::vector<StatDifference>* outDiffs)
            : groundItemVnum(vnum), outDifferences(outDiffs) {}
    };

    /**
     * @brief Zdarzenie rozgloszeniowe informujace, ze tooltip przedmiotu zostal porownany/zaktualizowany.
     */
    struct TooltipComparisonUpdatedEvent : public Core::IEvent
    {
        EterBase::ItemVnum vnum;
        
        explicit TooltipComparisonUpdatedEvent(EterBase::ItemVnum vnum) : vnum(vnum) {}
    };

    /**
     * @brief Pamięć podręczna (Cache) etykiet przedmiotów (tooltip) uproszczona pod drop na ziemi
     *        z obsluga porownywania statystyk.
     */
    class FastItemTooltipCache final : public IFastItemTooltipCache
    {
    public:
        FastItemTooltipCache() = default;
        ~FastItemTooltipCache() override = default;

        void CacheTooltip(EterBase::ItemVnum vnum, const FormattedTooltipData& data) override
        {
            {
                std::lock_guard<std::mutex> lock(cacheMutex_);
                cache_[vnum] = data;
            }
            
            EterBase::ModernLogger::Debug("Cached tooltip for item {}", vnum.get());
            
            Core::EventBus::GetInstance().Publish(TooltipComparisonUpdatedEvent{vnum});
        }

        std::optional<FormattedTooltipData> GetTooltip(EterBase::ItemVnum vnum) const override
        {
            FormattedTooltipData baseData;
            {
                std::lock_guard<std::mutex> lock(cacheMutex_);
                auto it = cache_.find(vnum);
                if (it == cache_.end())
                {
                    return std::nullopt;
                }
                baseData = it->second;
            }

            auto diffResult = FetchDifferences(vnum);
            if (diffResult && !diffResult->empty())
            {
                const auto& differences = diffResult.value();
                baseData.description += "\n\nPorównanie:";
                for (const auto& diff : differences)
                {
                    int32_t delta = diff.groundValue - diff.equippedValue;
                    if (delta > 0)
                    {
                        // Pozytywna roznica
                        baseData.description += std::format("\n{} |cFF00FF00(+{})|r", diff.statName, delta);
                    }
                    else if (delta < 0)
                    {
                        // Negatywna roznica
                        baseData.description += std::format("\n{} |cFFFF0000({})|r", diff.statName, delta);
                    }
                    else
                    {
                        // Brak roznicy
                        baseData.description += std::format("\n{} (0)", diff.statName);
                    }
                }
            }

            return baseData;
        }

        void Invalidate(EterBase::ItemVnum vnum) override
        {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            if (cache_.erase(vnum) > 0)
            {
                EterBase::ModernLogger::Debug("Invalidated tooltip cache for item {}", vnum.get());
            }
        }

        void ClearAll() override
        {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            cache_.clear();
            EterBase::ModernLogger::Info("Cleared all tooltip caches");
        }

    private:
        /**
         * @brief Pobiera różnice statystyk używając zdarzeń i Result C++23.
         */
        EterBase::Result<std::vector<StatDifference>> FetchDifferences(EterBase::ItemVnum vnum) const
        {
            std::vector<StatDifference> differences;
            Core::EventBus::GetInstance().Publish(ItemTooltipComparisonRequestEvent(vnum, &differences));
            return differences;
        }

        mutable std::mutex cacheMutex_;
        std::unordered_map<EterBase::ItemVnum, FormattedTooltipData> cache_;
    };

    /**
     * @brief Factory tworzące instancję w pełni ukrytą przed resztą kodu.
     */
    std::unique_ptr<IFastItemTooltipCache> CreateFastItemTooltipCache_Comparison()
    {
        return std::make_unique<FastItemTooltipCache>();
    }

} // namespace UserInterface::GroundDrop
