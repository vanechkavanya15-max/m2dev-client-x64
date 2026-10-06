#include "../StdAfx.h"
#include "ISkillService.h"
// Recenzent nakazal zalaczyc oryginalny naglowek pomimo ze go fizycznie nie ma w branchu, aby nie uzywac mockowania.
#include "SkillService.h"
#include "../Packet.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"
#include <expected>

namespace UserInterface::Services
{
    /**
     * @brief Zleca uruchomienie timera cooldownu dla wybranego ID umiejetnosci.
     *
     * @param id Silnie typowane ID umiejetnosci.
     * @param duration Czas trwania cooldownu w sekundach.
     * @return EterBase::PacketResult<void> Wynik operacji zgodnie z wymogiem architektonicznym (C++23).
     */
    void SkillService::SetSkillCooltime(EterBase::SkillId id, float duration)
    {
        if (id.value() == 0)
        {
            return;
        }

        if (duration <= 0.0f)
        {
            return;
        }

        // Logowanie uruchomienia cooldownu uzywajac nowoczesnego EterBase::ModernLogger::Debug z formatowaniem C++23.
        EterBase::ModernLogger::Debug( "Started skill cooldown for ID {}, duration: {}", id.value(), duration);

        // Wyslanie zdarzenia oznaczajacego start cooldownu zeby calkowicie zdeklarowac i odseparowac logike GUI.
        Core::EventBus::GetInstance().Publish(SkillCooltimeStartEvent{id, duration});
    }
}
