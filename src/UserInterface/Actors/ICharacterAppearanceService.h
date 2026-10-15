#pragma once

#include <cstdint>
#include "EterBase/StrongTypes.h"

namespace UserInterface::Actors
{
    /**
     * @brief Stan wizualny ekwipunku i wygladu postaci.
     */
    struct CharacterVisualPartView
    {
        uint32_t armorVnum{0};
        uint32_t weaponVnum{0};
        uint32_t hairVnum{0};
        uint32_t sashVnum{0};
        uint32_t effectFlags{0};
    };

    /**
     * @brief Interfejs zarzadzania czesciami wygladu aktora bez bezposredniego wiazania z Direct3D.
     */
    class ICharacterAppearanceService
    {
    public:
        virtual ~ICharacterAppearanceService() = default;

        virtual void SetArmor(EterBase::EntityId id, uint32_t armorVnum) = 0;
        virtual void SetWeapon(EterBase::EntityId id, uint32_t weaponVnum) = 0;
        virtual void SetHair(EterBase::EntityId id, uint32_t hairVnum) = 0;
        virtual void SetSash(EterBase::EntityId id, uint32_t sashVnum) = 0;
        virtual CharacterVisualPartView GetAppearance(EterBase::EntityId id) const = 0;
        virtual void RemoveCharacter(EterBase::EntityId id) = 0;
        virtual void Clear() = 0;
    };
}
