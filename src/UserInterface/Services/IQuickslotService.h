#pragma once

#include <cstdint>
#include <optional>
#include "../Core/EventBus.h"

namespace UserInterface::Services
{
    /**
     * @brief Zdarzenie rozglaszane po calkowitym wyczyszczeniu paska szybkiego dostepu (np. reset sesji).
     */
    struct QuickslotClearedEvent : public Core::IEvent
    {
        QuickslotClearedEvent() = default;
    };

    /**
     * @brief Definicja pojedynczego slotu w pasku szybkiego dostepu.
     */
    struct QuickslotView
    {
        uint8_t type{0};  // 0: pusty, 1: item, 2: skill, 3: komenda
        uint32_t pos{0};
    };

    /**
     * @brief Interfejs mikro-serwisu paska szybkiego dostepu (quickslot).
     */
    class IQuickslotService
    {
    public:
        virtual ~IQuickslotService() = default;

        virtual void SetQuickslot(uint8_t slotIndex, const QuickslotView& slot) = 0;
        virtual void DeleteQuickslot(uint8_t slotIndex) = 0;
        virtual void SwapQuickslots(uint8_t fromIndex, uint8_t toIndex) = 0;
        virtual std::optional<QuickslotView> GetQuickslot(uint8_t slotIndex) const = 0;
        virtual void Clear() = 0;
    };
}
