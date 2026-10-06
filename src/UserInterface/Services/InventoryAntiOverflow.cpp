#include "../StdAfx.h"
#include "IInventoryService.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../../UserInterface/Core/EventBus.h"
#include "../../UserInterface/Core/InventoryEvents.h"

#include <memory>
#include <optional>

namespace UserInterface::Services
{
    class InventoryAntiOverflow : public IInventoryService
    {
    public:
        explicit InventoryAntiOverflow(std::shared_ptr<IInventoryService> innerService)
            : m_innerService(std::move(innerService))
        {}

        virtual EterBase::PacketResult<void> SetItem(EterBase::ItemSlot slot, const InventoryItemView& item) override
        {
            if (item.count > 200)
            {
                EterBase::ModernLogger::Error("AntiOverflow: Attempted to set item count {} at slot {}, exceeding max limit of 200.", item.count, slot.get());
                
                // Publish an event indicating the failed / dropped item attempt (if applicable, using ItemDropped as a generic warning event since there isn't a dedicated error event).
                // It can also notify the UI that the acquisition failed.
                // However, the best semantic fit is to notify the bus.
                // We'll publish a generic drop event for the excessive amount, or just log.
                // The instructions requested: Szyna zdarzen UserInterface::Core::EventBus::GetInstance().Publish(...) do powiadamiania GUI.
                UserInterface::Core::EventBus::GetInstance().Publish(
                    std::make_shared<UserInterface::Core::InventoryEvents::ItemDropped>(item.vnum, slot, item.count)
                );

                return std::unexpected(EterBase::PacketError::MalformedPayload);
            }

            auto result = m_innerService->SetItem(slot, item);
            return result;
        }

        virtual EterBase::PacketResult<void> RemoveItem(EterBase::ItemSlot slot) override
        {
            return m_innerService->RemoveItem(slot);
        }

        virtual std::optional<InventoryItemView> GetItem(EterBase::ItemSlot slot) const override
        {
            return m_innerService->GetItem(slot);
        }

        virtual bool IsSlotEmpty(EterBase::ItemSlot slot) const override
        {
            return m_innerService->IsSlotEmpty(slot);
        }

        virtual bool IsItemLocked(EterBase::ItemSlot slot) const override
        {
            return m_innerService->IsItemLocked(slot);
        }

        virtual void SetItemLock(EterBase::ItemSlot slot, bool locked) override
        {
            m_innerService->SetItemLock(slot, locked);
        }

        virtual uint32_t GetItemCount(EterBase::ItemVnum vnum) const override
        {
            return m_innerService->GetItemCount(vnum);
        }

        virtual EterBase::PacketResult<void> SwapSlots(EterBase::ItemSlot from, EterBase::ItemSlot to) override
        {
            return m_innerService->SwapSlots(from, to);
        }

        virtual void Clear() override
        {
            m_innerService->Clear();
        }

    private:
        std::shared_ptr<IInventoryService> m_innerService;
    };

    // Factory function to instantiate the decorator
    std::shared_ptr<IInventoryService> CreateInventoryAntiOverflow(std::shared_ptr<IInventoryService> innerService)
    {
        return std::make_shared<InventoryAntiOverflow>(std::move(innerService));
    }
}
