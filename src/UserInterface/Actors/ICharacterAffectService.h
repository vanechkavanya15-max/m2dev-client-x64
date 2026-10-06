#pragma once

#include <cstdint>
#include "EterBase/StrongTypes.h"

namespace UserInterface::Actors
{
    /**
     * @brief Interfejs zarzadzania buffami, debuffami i afektami postaci w pamieci C++.
     */
    class ICharacterAffectService
    {
    public:
        virtual ~ICharacterAffectService() = default;

        virtual void SetAffect(EterBase::EntityId id, uint32_t affectIndex, bool enabled) = 0;
        virtual bool HasAffect(EterBase::EntityId id, uint32_t affectIndex) const = 0;
        virtual void ClearAffects(EterBase::EntityId id) = 0;
        virtual void Clear() = 0;
    };
}
