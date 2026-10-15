#include "../StdAfx.h"
#include "IFastItemTooltipCache.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/StrongTypes.h"
#include <unordered_map>
#include <mutex>
#include <optional>

namespace UserInterface::GroundDrop
{
    /**
     * @brief Zdarzenie publikowane, gdy podglad przedmiotu na ziemi jest gotowy do wyswietlenia.
     */
    struct GroundItemTooltipShowEvent : public Core::IEvent
    {
        EterBase::ItemVnum vnum;
        FormattedTooltipData data;

        GroundItemTooltipShowEvent(EterBase::ItemVnum vnum, const FormattedTooltipData& data)
            : vnum(vnum), data(data) {}
    };

    /**
     * @brief Implementacja pamieci podrecznej podgladu przedmiotow oparta o zdarzenia (EventBus).
     */
    class ItemTooltip_Events : public IFastItemTooltipCache
    {
    public:
        ~ItemTooltip_Events() override = default;

        /**
         * @brief Zapisuje dane podgladu w pamieci podrecznej i publikuje zdarzenie dla GUI.
         * @param vnum VNUM przedmiotu.
         * @param data Sformatowane dane podgladu.
         */
        void CacheTooltip(EterBase::ItemVnum vnum, const FormattedTooltipData& data) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cache[vnum] = data;

            EterBase::ModernLogger::Info("ItemTooltip_Events: Cached tooltip for item VNUM {}", static_cast<uint32_t>(vnum.value()));

            Core::EventBus::GetInstance().Publish(GroundItemTooltipShowEvent(vnum, data));
        }

        /**
         * @brief Pobiera zbuforowane dane podgladu przedmiotu.
         * @param vnum VNUM przedmiotu.
         * @return Opcjonalne sformatowane dane podgladu.
         */
        std::optional<FormattedTooltipData> GetTooltip(EterBase::ItemVnum vnum) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (auto it = m_cache.find(vnum); it != m_cache.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        /**
         * @brief Usuwa dany przedmiot z pamieci podrecznej.
         * @param vnum VNUM przedmiotu.
         */
        void Invalidate(EterBase::ItemVnum vnum) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_cache.erase(vnum) > 0)
            {
                EterBase::ModernLogger::Debug("ItemTooltip_Events: Invalidated tooltip for item VNUM {}", static_cast<uint32_t>(vnum.value()));
            }
        }

        /**
         * @brief Czysci cala pamiec podreczna.
         */
        void ClearAll() override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cache.clear();
            EterBase::ModernLogger::Info("ItemTooltip_Events: Cleared all tooltips from cache.");
        }

    private:
        mutable std::mutex m_mutex;
        std::unordered_map<EterBase::ItemVnum, FormattedTooltipData> m_cache;
    };
}
