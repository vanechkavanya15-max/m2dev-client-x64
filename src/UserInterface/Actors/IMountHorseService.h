#pragma once

#include <cstdint>
#include "EterBase/StrongTypes.h"

namespace UserInterface::Actors
{
    /**
     * @brief Interfejs zarzadzania stanem konia i wierzchowcow.
     */
    class IMountHorseService
    {
    public:
        virtual ~IMountHorseService() = default;

        virtual void Mount(EterBase::EntityId riderId, uint32_t mountVnum) = 0;
        virtual void Dismount(EterBase::EntityId riderId) = 0;
        virtual bool IsMounted(EterBase::EntityId riderId) const = 0;
        virtual uint32_t GetMountVnum(EterBase::EntityId riderId) const = 0;
        virtual void Clear() = 0;
    };
}
