#include "../StdAfx.h"
#include "IFastItemTooltipCache.h"
#include "../../GameLib/ItemManager.h"
#include "../../GameLib/ItemData.h"
#include "../Core/EventBus.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include <expected>

namespace UserInterface::GroundDrop
{
    /**
     * @brief Zdarzenie emitowane po udanym lub nieudanym obliczeniu ceny sprzedazy.
     */
    struct TooltipCostCalcEvent : public Core::IEvent
    {
        EterBase::ItemVnum vnum;
        uint32_t quantity;
        int32_t finalPrice;
        bool isSuccess;

        TooltipCostCalcEvent(EterBase::ItemVnum v, uint32_t q, int32_t p, bool s)
            : vnum(v), quantity(q), finalPrice(p), isSuccess(s)
        {
        }
    };

    /**
     * @brief Serwis do kalkulacji ceny sprzedazy u handlarza NPC z uwzglednieniem podatku (1/5).
     */
    class TooltipCostCalc
    {
    public:
        /**
         * @brief Oblicza cene sprzedazy danego przedmiotu i ilosci.
         * 
         * W C++ legacy cena byla obliczana z uwzglednieniem ITEM_FLAG_COUNT_PER_1GOLD 
         * i standardowego podatku wynoszacego 1/5 ceny bazowej.
         * 
         * @param vnum Vnum przedmiotu (silny typ)
         * @param quantity Ilosc przedmiotow (domyslnie 1)
         * @return std::expected<int32_t, EterBase::EntityError> Obliczona cena lub blad
         */
        static std::expected<int32_t, EterBase::EntityError> CalculateSellPrice(
            EterBase::ItemVnum vnum, 
            uint32_t quantity = 1)
        {
            CItemData* itemData = nullptr;
            if (!CItemManager::Instance().GetItemDataPointer(vnum.value(), &itemData) || !itemData)
            {
                EterBase::ModernLogger::Warning("TooltipCostCalc: Item {} not found.", vnum.value());
                
                Core::EventBus::GetInstance().Publish(
                    TooltipCostCalcEvent{vnum, quantity, 0, false}
                );
                
                return std::unexpected(EterBase::EntityError::NotFound);
            }

            int32_t basePrice = 0;
            
            if (itemData->IsFlag(CItemData::ITEM_FLAG_COUNT_PER_1GOLD))
            {
                if (itemData->GetISellItemPrice() > 0)
                {
                    basePrice = static_cast<int32_t>(quantity / itemData->GetISellItemPrice());
                }
            }
            else
            {
                basePrice = static_cast<int32_t>(itemData->GetISellItemPrice() * quantity);
            }

            // NPC tax is 1/5 in standard metin2
            int32_t finalPrice = basePrice / 5;

            EterBase::ModernLogger::Debug(
                "TooltipCostCalc: Item {} (qty: {}) sell price calculated to {}.",
                vnum.value(), quantity, finalPrice
            );

            Core::EventBus::GetInstance().Publish(
                TooltipCostCalcEvent{vnum, quantity, finalPrice, true}
            );

            return finalPrice;
        }
    };
}
