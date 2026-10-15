#pragma once

#include <cstdint>
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../Core/EventBus.h"
#include "../Packet.h"

namespace UserInterface::Actors
{
    /**
     * @brief Zdarzenie publikowane gdy zmienia sie ranga (tytul) postaci.
     */
    struct RankTitleChangedEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        int32_t alignment;
        uint32_t grade;
        
        RankTitleChangedEvent(EterBase::EntityId id, int32_t align, uint32_t grd)
            : entityId(id), alignment(align), grade(grd) {}
    };

    /**
     * @brief Interfejs uslugi kalkulacji i zarzadzania tytulami rangi (Alignment).
     */
    class IRankTitleService
    {
    public:
        virtual ~IRankTitleService() = default;

        /**
         * @brief Aktualizuje punkty rangi i publikuje zdarzenie jesli ranga sie zmienila.
         * @param id Identyfikator bytu.
         * @param points Punkty rangi (Alignment).
         */
        virtual EterBase::PacketResult<void> SetAlignment(EterBase::EntityId id, int32_t points) = 0;

        /**
         * @brief Zwraca aktualne punkty rangi.
         * @param id Identyfikator bytu.
         */
        virtual EterBase::Result<int32_t, EterBase::EntityError> GetAlignment(EterBase::EntityId id) const = 0;

        /**
         * @brief Zwraca aktualny stopien rangi (Grade).
         * @param id Identyfikator bytu.
         */
        virtual EterBase::Result<uint32_t, EterBase::EntityError> GetGrade(EterBase::EntityId id) const = 0;
    };
}
