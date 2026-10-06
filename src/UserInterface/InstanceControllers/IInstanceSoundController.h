#pragma once

#include <cstdint>
#include <string_view>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::InstanceControllers
{
    enum class SurfaceType : uint8_t
    {
        Grass = 0,
        Dirt,
        Stone,
        Wood,
        Water
    };

    class IInstanceSoundController
    {
    public:
        virtual ~IInstanceSoundController() = default;

        virtual void PlayFootstep(SurfaceType surface) = 0;
        virtual void PlayAttackSwing(uint32_t weaponType) = 0;
        virtual void PlayDamageHit(uint32_t damageType) = 0;
        virtual void PlayDeathCry() = 0;
        virtual void PlaySkillSound(uint32_t skillId) = 0;
        virtual void StopAllSounds() = 0;
    };
}
