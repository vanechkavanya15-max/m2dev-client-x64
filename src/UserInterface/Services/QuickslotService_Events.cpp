#include "../StdAfx.h"
#include "IQuickslotService.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include <array>
#include <optional>
#include <memory>

namespace UserInterface::Core {
    /**
     * @brief Zdarzenie aktualizacji paska szybkiego dostepu (Quickslot) rozglaszane na szynie.
     */
    struct QuickslotUpdateEvent : public IEvent {
        QuickslotUpdateEvent() = default;
    };
}

namespace UserInterface::Services {

/**
 * @brief Implementacja serwisu Quickslot oparta o zdarzenia C++23.
 * Zachowuje Single Responsibility Principle: tylko zarzadza slotami i powiadamia EventBus.
 */
class QuickslotService_Events final : public IQuickslotService {
public:
    QuickslotService_Events() {
        EterBase::ModernLogger::Info("QuickslotService_Events: Zainicjowano (C++23 Zero-Conflict).");
    }

    ~QuickslotService_Events() override {
        EterBase::ModernLogger::Info("QuickslotService_Events: Zniszczono.");
    }

    void SetQuickslot(uint8_t slotIndex, const QuickslotView& slot) override {
        if (auto result = TrySetQuickslot(EterBase::ItemSlot(slotIndex), slot); !result.has_value()) {
            EterBase::ModernLogger::Error("QuickslotService_Events::SetQuickslot - Blad: {}", EterBase::ToString(result.error()));
        } else {
            LogSlotUpdate(slotIndex, slot);
            PublishUpdateEvent();
        }
    }

    void DeleteQuickslot(uint8_t slotIndex) override {
        if (auto result = TryDeleteQuickslot(EterBase::ItemSlot(slotIndex)); !result.has_value()) {
            EterBase::ModernLogger::Error("QuickslotService_Events::DeleteQuickslot - Blad: {}", EterBase::ToString(result.error()));
        } else {
            EterBase::ModernLogger::Debug("QuickslotService_Events::DeleteQuickslot - Wyczyszczono slot {}.", slotIndex);
            PublishUpdateEvent();
        }
    }

    void SwapQuickslots(uint8_t fromIndex, uint8_t toIndex) override {
        if (auto result = TrySwapQuickslots(EterBase::ItemSlot(fromIndex), EterBase::ItemSlot(toIndex)); !result.has_value()) {
            EterBase::ModernLogger::Error("QuickslotService_Events::SwapQuickslots - Blad: {}", EterBase::ToString(result.error()));
        } else {
            EterBase::ModernLogger::Debug("QuickslotService_Events::SwapQuickslots - Zamieniono {} z {}.", fromIndex, toIndex);
            PublishUpdateEvent();
        }
    }

    std::optional<QuickslotView> GetQuickslot(uint8_t slotIndex) const override {
        if (slotIndex >= m_slots.size()) {
            EterBase::ModernLogger::Warn("QuickslotService_Events::GetQuickslot - Indeks {} poza zakresem.", slotIndex);
            return std::nullopt;
        }
        return m_slots[slotIndex];
    }

    void Clear() override {
        for (auto& slot : m_slots) {
            slot.reset();
        }
        EterBase::ModernLogger::Info("QuickslotService_Events::Clear - Wyczyszczono cache paska.");
        PublishUpdateEvent();
    }

private:
    EterBase::VoidResult<EterBase::InventoryError> TrySetQuickslot(EterBase::ItemSlot slotIndex, const QuickslotView& slot) {
        if (slotIndex.value() >= QUICKSLOT_MAX_NUM) {
            return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
        }
        m_slots[slotIndex.value()] = slot;
        return {};
    }

    EterBase::VoidResult<EterBase::InventoryError> TryDeleteQuickslot(EterBase::ItemSlot slotIndex) {
        if (slotIndex.value() >= QUICKSLOT_MAX_NUM) {
            return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
        }
        if (!m_slots[slotIndex.value()].has_value()) {
            return std::unexpected(EterBase::InventoryError::SlotEmpty);
        }
        m_slots[slotIndex.value()].reset();
        return {};
    }

    EterBase::VoidResult<EterBase::InventoryError> TrySwapQuickslots(EterBase::ItemSlot fromIndex, EterBase::ItemSlot toIndex) {
        if (fromIndex.value() >= QUICKSLOT_MAX_NUM || toIndex.value() >= QUICKSLOT_MAX_NUM) {
            return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
        }
        std::swap(m_slots[fromIndex.value()], m_slots[toIndex.value()]);
        return {};
    }

    void LogSlotUpdate(uint8_t slotIndex, const QuickslotView& slot) const {
        if (slot.type == 1) { // Type ITEM
            EterBase::ModernLogger::Debug("QuickslotService_Events::SetQuickslot - Slot {} -> Przedmiot (poz: {}).", 
                slotIndex, EterBase::ItemSlot(slot.pos).value());
        } else if (slot.type == 2) { // Type SKILL
            EterBase::ModernLogger::Debug("QuickslotService_Events::SetQuickslot - Slot {} -> Umiejetnosc (id: {}).", 
                slotIndex, EterBase::SkillId(slot.pos).value());
        } else {
            EterBase::ModernLogger::Debug("QuickslotService_Events::SetQuickslot - Slot {} -> Typ {}, Poz {}.", 
                slotIndex, slot.type, slot.pos);
        }
    }

    void PublishUpdateEvent() const {
        UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::QuickslotUpdateEvent{});
    }

    // Uzywa QUICKSLOT_MAX_NUM ze zdefiniowanych naglowkow pakietow
    std::array<std::optional<QuickslotView>, QUICKSLOT_MAX_NUM> m_slots;
};

std::unique_ptr<IQuickslotService> CreateQuickslotService_Events() {
    return std::make_unique<QuickslotService_Events>();
}

} // namespace UserInterface::Services
