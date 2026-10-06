#include "../StdAfx.h"
#include "IInventoryService.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include <memory>

namespace UserInterface::Services
{
    /**
     * @brief Zdarzenie emitowane po udanej zamianie dwoch slotow ekwipunku.
     */
    struct InventorySlotSwappedEvent : public Core::IEvent
    {
        EterBase::ItemSlot fromSlot;
        EterBase::ItemSlot toSlot;

        InventorySlotSwappedEvent(EterBase::ItemSlot from, EterBase::ItemSlot to)
            : fromSlot(from), toSlot(to) {}
    };

    /**
     * @brief Implementacja modulu wymiany ekwipunku jako osobnego serwisu rozszerzającego.
     * Hermetyzuje ODR chroniac globalna przestrzen, delegujac niezwiazane operacje.
     */
    class InventoryServiceSwap : public IInventoryService
    {
    private:
        IInventoryService& m_baseService;

    public:
        explicit InventoryServiceSwap(IInventoryService& baseService) : m_baseService(baseService) {}

        EterBase::PacketResult<void> SwapSlots(EterBase::ItemSlot from, EterBase::ItemSlot to) override
        {
            if (from == to)
            {
                EterBase::ModernLogger::Debug("InventoryServiceSwap::SwapSlots: Same slots {}, ignoring.", from.value());
                return {};
            }

            if (m_baseService.IsItemLocked(from) || m_baseService.IsItemLocked(to))
            {
                EterBase::ModernLogger::Error("InventoryServiceSwap::SwapSlots: Cannot swap locked items (from: {}, to: {})", from.value(), to.value());
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            auto itemFrom = m_baseService.GetItem(from);
            auto itemTo = m_baseService.GetItem(to);

            // Zabezpieczenie przed brakiem przedmiotow z dwoch stron 
            if (!itemFrom && !itemTo)
            {
                return {}; // Nic do zamiany
            }

            // Krok 1: Usun ze starego miejsca zeby zwolnic
            if (itemFrom) m_baseService.RemoveItem(from);
            if (itemTo) m_baseService.RemoveItem(to);

            bool successTo = true;
            bool successFrom = true;

            // Krok 2: Ustaw w nowych slotach (z kopii widokow)
            if (itemFrom)
            {
                itemFrom->slot = to;
                if (!m_baseService.SetItem(to, *itemFrom))
                {
                    successTo = false;
                }
            }

            if (itemTo)
            {
                itemTo->slot = from;
                if (!m_baseService.SetItem(from, *itemTo))
                {
                    successFrom = false;
                }
            }

            // Krok 3: Walidacja atomowa - jesli cos nie powiodlo sie, przywracamy stan poczatkowy
            if (!successTo || !successFrom)
            {
                EterBase::ModernLogger::Error("InventoryServiceSwap::SwapSlots: Failed to set items. Rolling back.");
                
                // Rollback powroty usun, by przywrocic prawidlowo
                m_baseService.RemoveItem(from);
                m_baseService.RemoveItem(to);
                
                if (itemFrom)
                {
                    itemFrom->slot = from;
                    m_baseService.SetItem(from, *itemFrom);
                }
                
                if (itemTo)
                {
                    itemTo->slot = to;
                    m_baseService.SetItem(to, *itemTo);
                }
                
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            EterBase::ModernLogger::Info("InventoryServiceSwap::SwapSlots: Successfully swapped {} and {}", from.value(), to.value());
            
            // Powiadomienie interfejsu (EventBus) z wlasciwym powiadomieniem
            UserInterface::Core::EventBus::GetInstance().Publish(InventorySlotSwappedEvent{from, to});

            return {};
        }

        // Delegacja pozostalych metod interfejsu 
        EterBase::PacketResult<void> SetItem(EterBase::ItemSlot slot, const InventoryItemView& item) override { return m_baseService.SetItem(slot, item); }
        EterBase::PacketResult<void> RemoveItem(EterBase::ItemSlot slot) override { return m_baseService.RemoveItem(slot); }
        std::optional<InventoryItemView> GetItem(EterBase::ItemSlot slot) const override { return m_baseService.GetItem(slot); }
        bool IsSlotEmpty(EterBase::ItemSlot slot) const override { return m_baseService.IsSlotEmpty(slot); }
        bool IsItemLocked(EterBase::ItemSlot slot) const override { return m_baseService.IsItemLocked(slot); }
        void SetItemLock(EterBase::ItemSlot slot, bool locked) override { m_baseService.SetItemLock(slot, locked); }
        uint32_t GetItemCount(EterBase::ItemVnum vnum) const override { return m_baseService.GetItemCount(vnum); }
        void Clear() override { m_baseService.Clear(); }
    };

    /**
     * @brief Factory method tworzaca serwis wymiany.
     */
    std::unique_ptr<IInventoryService> CreateInventorySwapService(IInventoryService& baseService)
    {
        return std::make_unique<InventoryServiceSwap>(baseService);
    }
}
