#include "../StdAfx.h"
#include "IInstanceSoundController.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../Core/EventBus.h"
#include "../InstanceBase.h"
#include "../../AudioLib/SoundEngine.h"

#include <cstdint>
#include <string_view>
#include <expected>
#include <format>
#include <string>

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Concrete implementation of IInstanceSoundController for Death Cry.
     * 
     * Handles death and falling sounds for different races and monsters.
     */
    class InstanceSound_DeathCry final : public IInstanceSoundController
    {
    public:
        explicit InstanceSound_DeathCry(CInstanceBase* instance) 
            : m_pkInstance(instance) 
        {
        }

        ~InstanceSound_DeathCry() override = default;

        void PlayFootstep(SurfaceType surface) override {}
        void PlayAttackSwing(uint32_t weaponType) override {}
        void PlayDamageHit(uint32_t damageType) override {}
        
        /**
         * @brief Plays the death cry sound for actors and falling sounds.
         * 
         * Determines the race and coordinates to play a 3D sound in the audio engine.
         */
        void PlayDeathCry() override
        {
            if (!m_pkInstance)
            {
                EterBase::ModernLogger::Error("Cannot play death cry without instance reference.");
                return;
            }

            uint32_t raceNum = m_pkInstance->GetRace();
            uint32_t vid = m_pkInstance->GetVirtualID();
            
            // Get position for 3D sound
            const TPixelPosition& pos = m_pkInstance->NEW_GetDstPixelPositionRef();

            std::string soundPath;
            if (raceNum < 10) 
            {
                soundPath = std::format("sound/pc/{}/dead.wav", raceNum);
            }
            else
            {
                soundPath = std::format("sound/monster/{}/dead.wav", raceNum);
            }
            
            std::string fallSoundPath = std::format("sound/common/fall_{}.wav", (raceNum % 3));

            EterBase::ModernLogger::Info("Playing death sounds: {} and {} for VID: {}", soundPath, fallSoundPath, vid);
            
            // Invoke the legacy SoundEngine singleton directly
            SoundEngine::Instance().PlaySound3D(soundPath, pos.x, pos.y, pos.z);
            SoundEngine::Instance().PlaySound3D(fallSoundPath, pos.x, pos.y, pos.z);

            // Also publish ActorDeadEvent as per modern C++23 standards for decoupled systems (like UI target boards, etc.)
            EterBase::EntityId entityId{vid};
            Core::EventBus::GetInstance().Publish(Core::ActorDeadEvent(entityId.value()));
        }

        void PlaySkillSound(uint32_t skillId) override {}
        void StopAllSounds() override {}

    private:
        CInstanceBase* m_pkInstance;
    };
    
    // Provide a way to create this concrete controller
    // According to C++23, we should use std::expected here to validate initialization,
    // though the constructor is simple, we adhere to the rule to return std::expected.
    std::expected<std::unique_ptr<IInstanceSoundController>, EterBase::EntityError> 
    CreateDeathCryController(CInstanceBase* instance)
    {
        if (!instance)
        {
            return std::unexpected(EterBase::EntityError::NotFound);
        }
        
        return std::make_unique<InstanceSound_DeathCry>(instance);
    }
}
