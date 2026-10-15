#pragma once

#include <cstdint>
#include <string_view>
#include <memory>
#include <expected>

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "../Core/EventBus.h"

class CInstanceBase;

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Typy nawierzchni podloza dla dzwiekow krokow.
     */
    enum class SurfaceType : uint8_t
    {
        Grass = 0,
        Dirt,
        Stone,
        Wood,
        Water
    };

    // --- Zdarzenia audio instancji ---

    struct AttackSwingSoundEvent : public Core::IEvent
    {
        EterBase::ItemVnum weaponType;
        std::string_view soundName;

        explicit AttackSwingSoundEvent(EterBase::ItemVnum weaponType, std::string_view soundName)
            : weaponType(weaponType), soundName(soundName) {}
    };

    struct FootstepSoundPlayedEvent : public Core::IEvent
    {
        SurfaceType surface;
        std::string_view soundName;

        FootstepSoundPlayedEvent(SurfaceType surface, std::string_view soundName)
            : surface(surface), soundName(soundName) {}
    };

    struct SoundHitImpactEvent : public Core::IEvent
    {
        uint32_t damageType;

        explicit SoundHitImpactEvent(uint32_t type) : damageType(type) {}
    };

    /**
     * @brief Glowny interfejs kontrolera dzwiekow instancji.
     */
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

    /**
     * @brief Modul wyzwalania dzwiekow zamachu bronia w zaleznosci od typu oreza.
     */
    class InstanceSound_AttackSwing
    {
    public:
        static EterBase::PacketResult<void> PlayAttackSwing(IInstanceSoundController& controller, uint32_t rawWeaponType);
    };

    /**
     * @brief Kontroler odtwarzania dzwiekow zgonu i upadku instancji (3D Audio).
     */
    class InstanceSound_DeathCry final : public IInstanceSoundController
    {
    public:
        explicit InstanceSound_DeathCry(CInstanceBase* instance);
        ~InstanceSound_DeathCry() override = default;

        void PlayFootstep(SurfaceType surface) override {}
        void PlayAttackSwing(uint32_t weaponType) override {}
        void PlayDamageHit(uint32_t damageType) override {}
        void PlayDeathCry() override;
        void PlaySkillSound(uint32_t skillId) override {}
        void StopAllSounds() override {}

    private:
        CInstanceBase* m_pkInstance{nullptr};
    };

    /**
     * @brief Kontroler dzwiekow krokow zaleznych od podloza.
     */
    class InstanceSound_Footsteps : public IInstanceSoundController
    {
    public:
        ~InstanceSound_Footsteps() override = default;

        void PlayFootstep(SurfaceType surface) override;
        void PlayAttackSwing(uint32_t weaponType) override;
        void PlayDamageHit(uint32_t damageType) override;
        void PlayDeathCry() override;
        void PlaySkillSound(uint32_t skillId) override;
        void StopAllSounds() override;

    private:
        EterBase::VoidResult<EterBase::EntityError> ProcessFootstep(SurfaceType surface);
    };

    /**
     * @brief Kontroler efektow akustycznych trafienia i uderzenia w cel.
     */
    class InstanceSoundHitImpactController : public IInstanceSoundController
    {
    public:
        InstanceSoundHitImpactController() = default;
        ~InstanceSoundHitImpactController() override = default;

        void PlayDamageHit(uint32_t damageType) override;
        void PlayFootstep(SurfaceType surface) override;
        void PlayAttackSwing(uint32_t weaponType) override;
        void PlayDeathCry() override;
        void PlaySkillSound(uint32_t skillId) override;
        void StopAllSounds() override;
    };

    // --- Fabryki ---
    std::expected<std::unique_ptr<IInstanceSoundController>, EterBase::EntityError>
    CreateDeathCryController(CInstanceBase* instance);
}
