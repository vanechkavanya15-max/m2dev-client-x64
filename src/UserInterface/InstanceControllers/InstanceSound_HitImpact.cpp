#include "../StdAfx.h"
#include "IInstanceSoundController.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Zdarzenie publikowane podczas uderzenia zadanego przeciwnikowi (Hit Impact).
     * Decouples sound logic from the underlying UI or audio sub-systems.
     */
    struct SoundHitImpactEvent : public Core::IEvent
    {
        uint32_t damageType;

        explicit SoundHitImpactEvent(uint32_t type) : damageType(type) {}
    };

    /**
     * @brief Modul odpowiedzialny wylacznie za dzwiek uderzenia ciala (Hit Impact).
     * Klasa zaimplementowana w pliku .cpp by uniknac globalnego zanieczyszczenia 
     * i trzymac sie SRP (Single Responsibility Principle) zgodnie z Wave 5.
     */
    class InstanceSoundHitImpactController : public IInstanceSoundController
    {
    public:
        InstanceSoundHitImpactController() = default;
        ~InstanceSoundHitImpactController() override = default;

        void PlayDamageHit(uint32_t damageType) override
        {
            EterBase::ModernLogger::Info("InstanceSoundHitImpactController::PlayDamageHit - Triggering hit sound for damageType: {}", damageType);

            SoundHitImpactEvent event(damageType);
            Core::EventBus::GetInstance().Publish(event);
        }

        // Stub methods to satisfy the IInstanceSoundController interface.
        // As per architecture rules, single responsibility isolates logic, so these do nothing but error log.

        void PlayFootstep(SurfaceType surface) override
        {
            EterBase::ModernLogger::Error("InstanceSoundHitImpactController::PlayFootstep - Not supported in this controller.");
        }

        void PlayAttackSwing(uint32_t weaponType) override
        {
            EterBase::ModernLogger::Error("InstanceSoundHitImpactController::PlayAttackSwing - Not supported in this controller.");
        }

        void PlayDeathCry() override
        {
            EterBase::ModernLogger::Error("InstanceSoundHitImpactController::PlayDeathCry - Not supported in this controller.");
        }

        void PlaySkillSound(uint32_t skillId) override
        {
            EterBase::ModernLogger::Error("InstanceSoundHitImpactController::PlaySkillSound - Not supported in this controller.");
        }

        void StopAllSounds() override
        {
            EterBase::ModernLogger::Error("InstanceSoundHitImpactController::StopAllSounds - Not supported in this controller.");
        }
    };
}
