#pragma once

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "../Core/EventBus.h"
#include <memory>

namespace UserInterface::GroundDrop
{
    /**
     * @brief Zdarzenie publikowane, gdy uplynie czas zycia przedmiotu na ziemi.
     * Subsystemy (takie jak GroundDropBatchRenderer) nasluchuja tego zdarzenia
     * aby bezpiecznie usunac instancje.
     */
    struct GroundDropExpiredEvent : public Core::IEvent
    {
        EterBase::EntityId virtualId;

        explicit GroundDropExpiredEvent(EterBase::EntityId virtualId) : virtualId(virtualId) {}
    };

    class IGroundDropExpirationTimer
    {
    public:
        virtual ~IGroundDropExpirationTimer() = default;

        virtual void SetExpirationTime(EterBase::EntityId virtualId, float durationSeconds) = 0;
        virtual EterBase::PacketResult<void> RemoveTimer(EterBase::EntityId virtualId) = 0;
        virtual void Update(float deltaTime) = 0;
        virtual void ClearAll() = 0;
    };

    // Fabryka do tworzenia instancji timera
    std::unique_ptr<IGroundDropExpirationTimer> CreateExpirationTimer();
}
