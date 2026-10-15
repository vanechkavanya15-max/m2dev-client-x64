#include "../StdAfx.h"
#include "ITargetHpBarService.h"
#include "src/UserInterface/Core/EventBus.h"
#include "src/UserInterface/Core/Events.h"
#include "src/UserInterface/Core/CombatEvents.h"
#include "src/EterBase/LogModern.h"

namespace UserInterface::TextTail
{
    /**
     * @brief Handler odpowiedzialny za zanikanie paska HP (TargetBoard) 
     * po odznaczeniu celu lub jego zgonie.
     */
    class TargetHpBarHideHandler
    {
    public:
        /**
         * @brief Rejestruje nasluchiwacze na szynie zdarzen (EventBus) i powiazuje je z usluga paska HP.
         * @param service Referencja do glownego serwisu ITargetHpBarService (Zero-Conflict).
         */
        static EterBase::PacketResult<void> BindEvents(ITargetHpBarService& service)
        {
            auto& eventBus = Core::EventBus::GetInstance();

            // Zanikanie paska po odznaczeniu celu (deselekcja / wylogowanie z pola widzenia).
            eventBus.Subscribe<::Core::Events::TargetDelete>([&service](const ::Core::Events::TargetDelete& ev) {
                EterBase::ModernLogger::Debug("TargetHpBar_Hide: Zdarzenie TargetDelete (ID: {}). Ukrywanie paska HP.", ev.targetId);
                service.HideTargetBar();
            });

            // Zanikanie paska w wyniku smierci bojowej.
            eventBus.Subscribe<Core::CombatEvents::TargetDied>([&service](const Core::CombatEvents::TargetDied& ev) {
                EterBase::ModernLogger::Debug("TargetHpBar_Hide: Zdarzenie TargetDied (ID: {}). Ukrywanie paska HP.", ev.targetId.value());
                service.HideTargetBar();
            });

            // Zanikanie paska w wyniku fizycznego zgonu aktora w swiecie gry.
            eventBus.Subscribe<Core::ActorDeadEvent>([&service](const Core::ActorDeadEvent& ev) {
                EterBase::ModernLogger::Debug("TargetHpBar_Hide: Zdarzenie ActorDeadEvent (ID: {}). Ukrywanie paska HP.", ev.entityId);
                service.HideTargetBar();
            });
            
            EterBase::ModernLogger::Info("TargetHpBar_Hide: Zainicjalizowano nasluchiwanie zdarzen zanikania paska HP.");
            return {};
        }
    };
}
