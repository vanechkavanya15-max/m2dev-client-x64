#include "../StdAfx.h"
#include "IQuickslotService.h"
#include "QuickslotService.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::Services
{
    /**
     * @brief Czysci caly pasek szybkiego dostepu i powiadamia UI (reset sesji).
     *
     * W architekturze decouplingowej (Domain Driven Design / CQRS) ten serwis 
     * publikuje intencje wyczyszczenia. Wlasciwy stan (np. QuickslotContainerModel) 
     * powinnien zablokowac / wyczyscic swoja zawartosc nasluchujac na to zdarzenie.
     */
    void QuickslotService::Clear()
    {
        EterBase::ModernLogger::Info("Rozpoczeto czyszczenie paska szybkiego dostepu (reset sesji).");

        // Rozeslanie zdarzenia o wyczyszczeniu Quickslotow. 
        Core::EventBus::GetInstance().Publish(QuickslotClearedEvent{});

        EterBase::ModernLogger::Info("Pasek szybkiego dostepu zostal pomyslnie wyczyszczony.");
    }
}
