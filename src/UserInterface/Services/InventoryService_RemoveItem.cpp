#include "../StdAfx.h"
#include "IInventoryService.h"
#include "../Packet.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "../Domain/InventoryModel.h"

namespace UserInterface::Services
{
    /**
     * @brief Zdarzenie informujace o usunieciu przedmiotu z ekwipunku (decoupling UI).
     */
    struct InventoryItemRemovedEvent : public Core::IEvent
    {
        EterBase::ItemSlot slot;
        explicit InventoryItemRemovedEvent(EterBase::ItemSlot slot) : slot(slot) {}
    };

    /**
     * @brief Dedykowany, pojedynczy serwis implementujacy akcje RemoveItem,
     * dzialajacy zgodnie z Zero-Conflict (brak modyfikacji cudzych naglowkow).
     */
    class InventoryService_RemoveItem : public IInventoryService
    {
    public:
        explicit InventoryService_RemoveItem(std::shared_ptr<InventoryModel> model) : m_model(model) {}
        virtual ~InventoryService_RemoveItem() = default;

        EterBase::PacketResult<void> RemoveItem(EterBase::ItemSlot slot) override
        {
            if (!m_model) {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            auto itemOpt = m_model->getItem(slot.get());
            if (!itemOpt.has_value()) {
                EterBase::ModernLogger::Debug("InventoryService_RemoveItem::RemoveItem: Slot {} is already empty.", slot.get());
                return {};
            }

            if (itemOpt->isLocked) {
                EterBase::ModernLogger::Warn("InventoryService_RemoveItem::RemoveItem: Cannot remove locked item from slot {}.", slot.get());
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            if (!m_model->clearItem(slot.get())) {
                EterBase::ModernLogger::Error("InventoryService_RemoveItem::RemoveItem: Failed to clear buffer for slot {}.", slot.get());
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            EterBase::ModernLogger::Info("InventoryService_RemoveItem::RemoveItem: Successfully removed item from slot {}.", slot.get());
            UserInterface::Core::EventBus::GetInstance().Publish(InventoryItemRemovedEvent{slot});

            return {};
        }

        // ====================================================================
        // Stubs dla pozostalych metod interfejsu (serwis jednofunkcyjny)
        // ====================================================================
        EterBase::PacketResult<void> SetItem(EterBase::ItemSlot slot, const InventoryItemView& item) override { return {}; }
        std::optional<InventoryItemView> GetItem(EterBase::ItemSlot slot) const override { return std::nullopt; }
        bool IsSlotEmpty(EterBase::ItemSlot slot) const override { return true; }
        bool IsItemLocked(EterBase::ItemSlot slot) const override { return false; }
        void SetItemLock(EterBase::ItemSlot slot, bool locked) override {}
        uint32_t GetItemCount(EterBase::ItemVnum vnum) const override { return 0; }
        EterBase::PacketResult<void> SwapSlots(EterBase::ItemSlot from, EterBase::ItemSlot to) override { return {}; }
        void Clear() override {}

    private:
        std::shared_ptr<InventoryModel> m_model;
    };
}
