#include "../StdAfx.h"
#include "IMountHorseService.h"
#include "../Core/EventBus.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::Actors
{
    /**
     * @brief Odspawnowuje model wierzchowca po wejsciu w strefy zablokowane.
     * @param mountService Referencja do uslugi zarzadzania wierzchowcami.
     * @param riderId Identyfikator postaci, ktora wymuszenie schodzi z wierzchowca.
     * @return Result<void, EntityError> - w przypadku bledu (np. postac nie uzywa wierzchowca) zwraca blad, w przeciwnym razie pusty wynik.
     */
    EterBase::Result<void, EterBase::EntityError> DespawnBlockedZoneMount(
        IMountHorseService& mountService,
        EterBase::EntityId riderId)
    {
        if (!mountService.IsMounted(riderId))
        {
            EterBase::ModernLogger::Debug("DespawnBlockedZoneMount: Rider {} is not mounted. Skipping.", riderId.value());
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        EterBase::ModernLogger::Info("DespawnBlockedZoneMount: Force dismounting rider {} due to blocked zone.", riderId.value());
        
        // Zdejmij gracza z wierzchowca
        mountService.Dismount(riderId);

        // Rozglos zdarzenie w swiecie gry (0, 0 poniewaz nie ma wierzchowca)
        Core::EventBus::GetInstance().Publish(Core::MountStateChangedEvent(riderId.value(), 0, 0));

        return {};
    }
}
