#pragma once

#include <cstdint>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::InstanceControllers
{
    class IInstanceMountHorseController
    {
    public:
        virtual ~IInstanceMountHorseController() = default;

        virtual EterBase::PacketResult<void> Mount(EterBase::EntityId mountVid, uint32_t mountVnum) = 0;
        virtual EterBase::PacketResult<void> Dismount() = 0;
        virtual bool IsMounted() const = 0;
        virtual EterBase::EntityId GetMountVID() const = 0;
        virtual uint32_t GetMountVnum() const = 0;
        virtual void UpdateMountTransform(float x, float y, float z, float rot) = 0;
        virtual void Clear() = 0;
    };
}
