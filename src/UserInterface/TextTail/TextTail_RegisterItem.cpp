#include "../StdAfx.h"
#include "ITextTailService.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace UserInterface::TextTail
{
    /**
     * @brief Zdarzenie informujace o zarejestrowaniu etykiety lezacego przedmiotu (decoupling UI).
     */
    struct ItemTailRegisteredEvent : public Core::IEvent
    {
        uint32_t virtualId;
        std::string name;

        ItemTailRegisteredEvent(uint32_t virtualId, std::string_view name)
            : virtualId(virtualId), name(name) {}
    };

    /**
     * @brief Dedykowany, pojedynczy serwis implementujacy akcje RegisterItemTail,
     * dzialajacy zgodnie z Zero-Conflict (brak modyfikacji cudzych naglowkow).
     */
    class TextTailService_RegisterItem : public ITextTailService
    {
    public:
        TextTailService_RegisterItem() = default;
        virtual ~TextTailService_RegisterItem() = default;

        /**
         * @brief Rejestruje etykiete dla lezacego przedmiotu (ze wskazaniem nazwy i ewentualnego wlasciciela).
         * @param virtualId Identyfikator (VID) przedmiotu na mapie.
         * @param name Nazwa przedmiotu (wraz z wlascicielem, sformatowana wczesniej jesli potrzeba).
         * @return EterBase::PacketResult<void> Sukces lub blad.
         */
        EterBase::PacketResult<void> RegisterItemTail(uint32_t virtualId, std::string_view name) override
        {
            if (name.empty()) {
                EterBase::ModernLogger::Error("TextTailService_RegisterItem: Cannot register item tail with empty name for VID {}.", virtualId);
                return std::unexpected(EterBase::PacketError::MalformedPayload);
            }

            EterBase::ModernLogger::Info("TextTailService_RegisterItem: Registering item tail for VID {} with name '{}'.", virtualId, name);

            // Powiadomienie innych systemow (np. renderowania TextTail UI) bezposrednio o nowym elemencie.
            Core::EventBus::GetInstance().Publish(ItemTailRegisteredEvent{virtualId, name});

            return {};
        }

        // ====================================================================
        // Stubs dla pozostalych metod interfejsu (serwis jednofunkcyjny)
        // ====================================================================
        EterBase::PacketResult<void> RegisterActorTail(const TextTailCreateData& data) override { return {}; }
        void RemoveTail(uint32_t virtualId) override {}
        void UpdateScreenPositions(float viewMatrix[16], float projMatrix[16]) override {}
        void RenderBatch() override {}
        void ClearAll() override {}
    };
}
