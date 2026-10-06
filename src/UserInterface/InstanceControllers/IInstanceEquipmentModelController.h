#pragma once

#include <cstdint>
#include <string_view>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::InstanceControllers
{
    enum class ModelPart : uint8_t
    {
        Main = 0,
        Weapon,
        WeaponLeft,
        Hair,
        Sash,
        CostumeBody,
        CostumeHair,
        CostumeWeapon,
        MaxParts
    };

    class IInstanceEquipmentModelController
    {
    public:
        virtual ~IInstanceEquipmentModelController() = default;

        virtual EterBase::PacketResult<void> SetPart(ModelPart part, EterBase::ItemVnum vnum) = 0;
        virtual EterBase::PacketResult<void> ClearPart(ModelPart part) = 0;
        virtual EterBase::ItemVnum GetPartVnum(ModelPart part) const = 0;
        virtual void SetLODLevel(uint8_t lodLevel) = 0;
        virtual void ClearAllParts() = 0;
    };
}
