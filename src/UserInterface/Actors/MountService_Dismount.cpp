#include "../StdAfx.h"
#include "IMountHorseService.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "Core/EventBus.h"
#include "EterBase/Result.h"
#include "../Packet.h"

namespace UserInterface::Actors::MountService
{
    /**
     * @brief Zdejmuje gracza (lub inna encje) z wierzchowca uzywajac wstrzyknietej uslugi.
     * 
     * Implementuje kontrakt SRP i Zasady Zero-Conflict, enkapsulujac wylacznie logike zsiadania.
     * Zwraca nowoczesny typ std::expected zgodny z C++23.
     * 
     * @param riderId Identyfikator encji, ktora zsiada.
     * @param service Referencja do modulu zarzadzajacego stanem (IMountHorseService).
     * @return EterBase::VoidResult<EterBase::EntityError> 
     */
    EterBase::VoidResult<EterBase::EntityError> MountDismountActor(EterBase::EntityId riderId, IMountHorseService& service)
    {
        if (!service.IsMounted(riderId))
        {
            EterBase::ModernLogger::Warning("Zsiadanie zablokowane: Entity {} nie uzywa zadnego wierzchowca.", riderId.value());
            return std::unexpected(EterBase::EntityError::InvalidType);
        }

        EterBase::ItemVnum currentMountVnum{ service.GetMountVnum(riderId) };
        EterBase::ModernLogger::Info("Rozpoczeto zsiadanie z wierzchowca {} dla Entity {}.", currentMountVnum.value(), riderId.value());

        // Aktualizacja stanu poprzez serwis
        service.Dismount(riderId);

        // Wyslanie zdarzenia o zsiadaniu (vnum = 0, pos = 0 oznacza zsiadanie)
        Core::EventBus::GetInstance().Publish(Core::MountStateChangedEvent(riderId.value(), 0, 0));

        EterBase::ModernLogger::Debug("Zakonczono zsiadanie dla Entity {}.", riderId.value());
        
        return {};
    }
}
