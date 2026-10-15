#include "../StdAfx.h"
#include "ICharacterAppearanceService.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "Core/EventBus.h"

namespace UserInterface::Actors
{
    /**
     * @brief Zdarzenie emitowane po wyczyszczeniu bazy wygladu postaci.
     * Zgodnie ze scisla zasada 'Zero-conflict', definiujemy to lokalnie, 
     * by nie modyfikowac globalnego EventBus.h.
     */
    struct AppearanceClearedEvent : public Core::IEvent
    {
    };

    /**
     * @brief Handler do resetu bazy wygladu postaci.
     */
    class AppearanceServiceClearHandler
    {
    public:
        static EterBase::PacketResult<void> Clear(ICharacterAppearanceService& service)
        {
            service.Clear();

            EterBase::ModernLogger::Info("AppearanceServiceClearHandler: Baza wygladu postaci zostala wyczyszczona.");

            // Informujemy moduly GUI / D3D o resecie stanow poprzez szyne zdarzen (EventBus)
            Core::EventBus::GetInstance().Publish(AppearanceClearedEvent{});

            return {};
        }
    };
}
