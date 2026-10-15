#include "../StdAfx.h"
#include "InstanceSoundControllers.h"
#include "../InstanceBase.h"
#include "GameLib/ItemData.h"
#include "AudioLib/SoundEngine.h"
#include "../../EterBase/LogModern.h"

#include <format>
#include <string>

namespace UserInterface::InstanceControllers
{
    // =========================================================================
    // InstanceSound_AttackSwing
    // =========================================================================

    EterBase::PacketResult<void> InstanceSound_AttackSwing::PlayAttackSwing(IInstanceSoundController& /*controller*/, uint32_t rawWeaponType)
    {
        EterBase::ItemVnum weaponType{rawWeaponType};
        std::string_view soundFile;

        switch (weaponType.value())
        {
            case CItemData::WEAPON_SWORD:
            case CItemData::WEAPON_TWO_HANDED:
                soundFile = "sound/pc/warrior/sword_swing.wav";
                break;
            case CItemData::WEAPON_DAGGER:
                soundFile = "sound/pc/assassin/dagger_swing.wav";
                break;
            case CItemData::WEAPON_BELL:
                soundFile = "sound/pc/shaman/bell_swing.wav";
                break;
            case CItemData::WEAPON_FAN:
                soundFile = "sound/pc/shaman/fan_swing.wav";
                break;
            case CItemData::WEAPON_BOW:
            case CItemData::WEAPON_ARROW:
                soundFile = "sound/pc/assassin/bow_swing.wav";
                break;
            default:
                soundFile = "sound/pc/warrior/sword_swing.wav";
                break;
        }

        if (!soundFile.empty())
        {
            SoundEngine::Instance().PlaySound2D(std::string(soundFile));
            EterBase::ModernLogger::Info("Played attack swing sound: {} for weapon type: {}", soundFile, weaponType.value());

            AttackSwingSoundEvent event(weaponType, soundFile);
            UserInterface::Core::EventBus::GetInstance().Publish(event);
        }

        return {};
    }

    // =========================================================================
    // InstanceSound_DeathCry
    // =========================================================================

    InstanceSound_DeathCry::InstanceSound_DeathCry(CInstanceBase* instance)
        : m_pkInstance(instance)
    {
    }

    void InstanceSound_DeathCry::PlayDeathCry()
    {
        if (!m_pkInstance)
        {
            EterBase::ModernLogger::Error("Cannot play death cry without instance reference.");
            return;
        }

        uint32_t raceNum = m_pkInstance->GetRace();
        uint32_t vid = m_pkInstance->GetVirtualID();

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

        SoundEngine::Instance().PlaySound3D(soundPath, pos.x, pos.y, pos.z);
        SoundEngine::Instance().PlaySound3D(fallSoundPath, pos.x, pos.y, pos.z);

        EterBase::EntityId entityId{vid};
        Core::EventBus::GetInstance().Publish(Core::ActorDeadEvent(entityId.value()));
    }

    // =========================================================================
    // InstanceSound_Footsteps
    // =========================================================================

    void InstanceSound_Footsteps::PlayFootstep(SurfaceType surface)
    {
        auto result = ProcessFootstep(surface);
        if (!result.has_value())
        {
            EterBase::ModernLogger::Error("Failed to play footstep sound: {}", EterBase::ToString(result.error()));
            return;
        }
    }

    void InstanceSound_Footsteps::PlayAttackSwing(uint32_t /*weaponType*/)
    {
        EterBase::ModernLogger::Trace("InstanceSound_Footsteps::PlayAttackSwing ignored (SRP)");
    }

    void InstanceSound_Footsteps::PlayDamageHit(uint32_t /*damageType*/)
    {
        EterBase::ModernLogger::Trace("InstanceSound_Footsteps::PlayDamageHit ignored (SRP)");
    }

    void InstanceSound_Footsteps::PlayDeathCry()
    {
        EterBase::ModernLogger::Trace("InstanceSound_Footsteps::PlayDeathCry ignored (SRP)");
    }

    void InstanceSound_Footsteps::PlaySkillSound(uint32_t /*skillId*/)
    {
        EterBase::ModernLogger::Trace("InstanceSound_Footsteps::PlaySkillSound ignored (SRP)");
    }

    void InstanceSound_Footsteps::StopAllSounds()
    {
        EterBase::ModernLogger::Trace("InstanceSound_Footsteps::StopAllSounds ignored (SRP)");
    }

    EterBase::VoidResult<EterBase::EntityError> InstanceSound_Footsteps::ProcessFootstep(SurfaceType surface)
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

    // =========================================================================
    // InstanceSoundHitImpactController
    // =========================================================================

    void InstanceSoundHitImpactController::PlayDamageHit(uint32_t damageType)
    {
        EterBase::ModernLogger::Info("InstanceSoundHitImpactController::PlayDamageHit - Triggering hit sound for damageType: {}", damageType);

        SoundHitImpactEvent event(damageType);
        Core::EventBus::GetInstance().Publish(event);
    }

    void InstanceSoundHitImpactController::PlayFootstep(SurfaceType /*surface*/)
    {
        EterBase::ModernLogger::Error("InstanceSoundHitImpactController::PlayFootstep - Not supported in this controller.");
    }

    void InstanceSoundHitImpactController::PlayAttackSwing(uint32_t /*weaponType*/)
    {
        EterBase::ModernLogger::Error("InstanceSoundHitImpactController::PlayAttackSwing - Not supported in this controller.");
    }

    void InstanceSoundHitImpactController::PlayDeathCry()
    {
        EterBase::ModernLogger::Error("InstanceSoundHitImpactController::PlayDeathCry - Not supported in this controller.");
    }

    void InstanceSoundHitImpactController::PlaySkillSound(uint32_t /*skillId*/)
    {
        EterBase::ModernLogger::Error("InstanceSoundHitImpactController::PlaySkillSound - Not supported in this controller.");
    }

    void InstanceSoundHitImpactController::StopAllSounds()
    {
        EterBase::ModernLogger::Error("InstanceSoundHitImpactController::StopAllSounds - Not supported in this controller.");
    }

    // =========================================================================
    // Fabryki
    // =========================================================================

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
