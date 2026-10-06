#include "../StdAfx.h"
#include "IInstanceSoundController.h"
#include "../Core/EventBus.h"
#include "../../EterBase/ModernLogger.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include <string_view>

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Zdarzenie wyzwalane przy odtwarzaniu dzwieku krokow (odseparowanie od GUI).
     */
    struct FootstepSoundPlayedEvent : public Core::IEvent
    {
        SurfaceType surface;
        std::string_view soundName;

        FootstepSoundPlayedEvent(SurfaceType surface, std::string_view soundName)
            : surface(surface), soundName(soundName) {}
    };

    /**
     * @brief Implementacja kontrolera dzwiekow skupiona na obsludze krokow (Zero-Conflict).
     */
    class InstanceSound_Footsteps : public IInstanceSoundController
    {
    public:
        virtual ~InstanceSound_Footsteps() = default;

        /**
         * @brief Odtwarza dzwiek kroku na podstawie typu podloza.
         * @param surface Typ podloza (trawa, ziemia, kamien, woda, drewno).
         */
        void PlayFootstep(SurfaceType surface) override
        {
            auto result = ProcessFootstep(surface);
            if (!result.has_value())
            {
                EterBase::ModernLogger::Error("Failed to play footstep sound: {}", EterBase::ToString(result.error()));
                return;
            }
        }

        // --- Empty stubs for unrelated actions, required by IInstanceSoundController contract ---
        void PlayAttackSwing(uint32_t weaponType) override
        {
            EterBase::ModernLogger::Trace("InstanceSound_Footsteps::PlayAttackSwing ignored (SRP)");
        }

        void PlayDamageHit(uint32_t damageType) override
        {
            EterBase::ModernLogger::Trace("InstanceSound_Footsteps::PlayDamageHit ignored (SRP)");
        }

        void PlayDeathCry() override
        {
            EterBase::ModernLogger::Trace("InstanceSound_Footsteps::PlayDeathCry ignored (SRP)");
        }

        void PlaySkillSound(uint32_t skillId) override
        {
            EterBase::ModernLogger::Trace("InstanceSound_Footsteps::PlaySkillSound ignored (SRP)");
        }

        void StopAllSounds() override
        {
            EterBase::ModernLogger::Trace("InstanceSound_Footsteps::StopAllSounds ignored (SRP)");
        }

    private:
        /**
         * @brief Validates surface type and maps it to a sound name.
         * @param surface Type of surface.
         * @return EterBase::VoidResult<EterBase::EntityError> on error.
         */
        EterBase::VoidResult<EterBase::EntityError> ProcessFootstep(SurfaceType surface)
        {
            std::string_view soundName;

            switch (surface)
            {
                case SurfaceType::Grass:
                    soundName = "sound/pc/footstep/grass.wav";
                    break;
                case SurfaceType::Dirt:
                    soundName = "sound/pc/footstep/dirt.wav";
                    break;
                case SurfaceType::Stone:
                    soundName = "sound/pc/footstep/stone.wav";
                    break;
                case SurfaceType::Wood:
                    soundName = "sound/pc/footstep/wood.wav";
                    break;
                case SurfaceType::Water:
                    soundName = "sound/pc/footstep/water.wav";
                    break;
                default:
                    return EterBase::MakeError(EterBase::EntityError::InvalidType);
            }

            EterBase::ModernLogger::Debug("Playing footstep sound for surface: {}", static_cast<uint32_t>(surface));
            
            Core::EventBus::GetInstance().Publish(FootstepSoundPlayedEvent{surface, soundName});

            return {};
        }
    };
} // namespace UserInterface::InstanceControllers
