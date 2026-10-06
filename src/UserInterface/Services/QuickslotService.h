#pragma once

#include "IQuickslotService.h"
#include <unordered_map>
#include <optional>

namespace UserInterface::Services
{
    class QuickslotService : public IQuickslotService
    {
    public:
        QuickslotService() = default;
        ~QuickslotService() override = default;

        void SetQuickslot(uint8_t slotIndex, const QuickslotView& slot) override {
            m_slots[slotIndex] = slot;
        }
        void DeleteQuickslot(uint8_t slotIndex) override {
            m_slots.erase(slotIndex);
        }
        void SwapQuickslots(uint8_t fromIndex, uint8_t toIndex) override {
            std::swap(m_slots[fromIndex], m_slots[toIndex]);
        }
        std::optional<QuickslotView> GetQuickslot(uint8_t slotIndex) const override {
            auto it = m_slots.find(slotIndex);
            if (it != m_slots.end()) return it->second;
            return std::nullopt;
        }
        void Clear() override;

    private:
        std::unordered_map<uint8_t, QuickslotView> m_slots;
    };
}
