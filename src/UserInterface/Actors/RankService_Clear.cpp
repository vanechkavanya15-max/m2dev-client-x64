#include "../StdAfx.h"
#include "RankTitleService.h"
#include "../../Packet.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

namespace UserInterface::Core {
    /**
     * @brief Event rozglaszany przez EventBus po wyczyszczeniu danych rang i gildii.
     */
    struct RankAndGuildClearedEvent : public IEvent {
        RankAndGuildClearedEvent() = default;
    };
}

namespace UserInterface::Actors {

    /**
     * @brief Wykonuje czyszczenie pamieci powiazanej z rangami i gildiami.
     * 
     * Publikuje zdarzenie RankAndGuildClearedEvent do EventBusa umozliwiajac
     * odciecie logiki domenowej od warstwy GUI zgodnie z Single Responsibility Principle.
     * 
     * @return EterBase::PacketResult<void> Sukces operacji
     */
    EterBase::PacketResult<void> RankTitleService::Clear() noexcept {
        // Czyszczenie stanow domenowych
        m_ranks.clear();
        m_guilds.clear();

        // Powiadomienie subskrybentow o wyczyszczeniu danych (EventBus)
        Core::EventBus::GetInstance().Publish(Core::RankAndGuildClearedEvent{});

        // Modern logowanie zdarzenia C++23
        EterBase::ModernLogger::Info("RankTitleService::Clear: Pomyslnie wyczyszczono pamiec rang i gildii.");
        
        return {};
    }

} // namespace UserInterface::Actors
