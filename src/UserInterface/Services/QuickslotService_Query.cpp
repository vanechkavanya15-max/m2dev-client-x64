#include "../StdAfx.h"
#include "IQuickslotService.h"
#include "../Packet.h"
#include "../PythonPlayer.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

#include <vector>
#include <memory>
#include <expected>

namespace UserInterface::Services
{
    /**
     * @brief Zdarzenie rozglaszajace zaladowanie pelnej konfiguracji paska szybkiego dostepu do GUI.
     */
    struct QuickslotConfigurationLoadedEvent : public Core::IEvent
    {
        std::vector<std::optional<QuickslotView>> slots;

        explicit QuickslotConfigurationLoadedEvent(std::vector<std::optional<QuickslotView>> s)
            : slots(std::move(s)) {}
    };

    /**
     * @brief Implementacja czesci Query serwisu QuickslotService.
     * Czesci Command (mutujace) zglaszaja blad zgodnosci z zasada CQRS.
     */
    class QuickslotService_Query final : public IQuickslotService
    {
    public:
        QuickslotService_Query() = default;
        ~QuickslotService_Query() override = default;

        // ====================================================================
        // IQuickslotService (Command) - Niewspierane w tym pliku (CQRS)
        // ====================================================================
        void SetQuickslot(uint8_t slotIndex, const QuickslotView& slot) override
        {
            EterBase::ModernLogger::Error("QuickslotService_Query::SetQuickslot called - CQRS violation");
        }

        void DeleteQuickslot(uint8_t slotIndex) override
        {
            EterBase::ModernLogger::Error("QuickslotService_Query::DeleteQuickslot called - CQRS violation");
        }

        void SwapQuickslots(uint8_t fromIndex, uint8_t toIndex) override
        {
            EterBase::ModernLogger::Error("QuickslotService_Query::SwapQuickslots called - CQRS violation");
        }

        void Clear() override
        {
            EterBase::ModernLogger::Error("QuickslotService_Query::Clear called - CQRS violation");
        }

        // ====================================================================
        // IQuickslotService (Query)
        // ====================================================================
        
        /**
         * @brief Pobiera informacje o pojedynczym slocie paska szybkiego dostepu.
         * @param slotIndex Indeks slotu (0-35).
         * @return Informacja o slocie (std::optional).
         */
        [[nodiscard]] std::optional<QuickslotView> GetQuickslot(uint8_t slotIndex) const override
        {
            if (slotIndex >= QUICKSLOT_MAX_NUM) {
                EterBase::ModernLogger::Warning("QuickslotService_Query::GetQuickslot - index {} out of range", slotIndex);
                return std::nullopt;
            }

            DWORD type = 0;
            DWORD pos = 0;
            CPythonPlayer::Instance().GetGlobalQuickSlotData(slotIndex, &type, &pos);

            if (type == 0) { // 0: pusty (None)
                return std::nullopt;
            }

            QuickslotView view;
            view.type = static_cast<uint8_t>(type);
            view.pos = pos;

            // Uzycie silnych typow tylko do bezpiecznego logowania (type safety)
            if (view.type == 1) { // Typ 1: Przedmiot
                EterBase::ModernLogger::Trace("Quickslot {} contains Item in slot {}", 
                    slotIndex, EterBase::ItemSlot(static_cast<uint16_t>(view.pos)).get());
            } 
            else if (view.type == 2) { // Typ 2: Umiejetnosc
                EterBase::ModernLogger::Trace("Quickslot {} contains Skill {}", 
                    slotIndex, EterBase::SkillId(view.pos).get());
            }

            return view;
        }

        // ====================================================================
        // Wymagania dodatkowe: Pobieranie konfiguracji calego paska szybkiego wyboru.
        // ====================================================================
        
        /**
         * @brief Pobiera konfiguracje calego paska i rozglasza ja do GUI przy uzyciu EventBus.
         * @return Oczekiwany rezultat (sukces lub blad operacji).
         */
        std::expected<void, std::string> PublishAllQuickslots() const
        {
            std::vector<std::optional<QuickslotView>> slots;
            slots.reserve(QUICKSLOT_MAX_NUM);

            for (uint8_t i = 0; i < QUICKSLOT_MAX_NUM; ++i) {
                slots.push_back(GetQuickslot(i));
            }

            Core::EventBus::Instance().Publish(QuickslotConfigurationLoadedEvent{std::move(slots)});
            EterBase::ModernLogger::Info("Published entire quickslot configuration to GUI ({} slots).", static_cast<int>(QUICKSLOT_MAX_NUM));
            
            return {};
        }
    };

    /**
     * @brief Fabryka do tworzenia instancji czesci Query serwisu.
     */
    std::unique_ptr<IQuickslotService> CreateQuickslotService_Query()
    {
        return std::make_unique<QuickslotService_Query>();
    }
}
