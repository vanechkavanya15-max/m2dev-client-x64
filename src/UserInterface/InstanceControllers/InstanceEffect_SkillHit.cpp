#include "../StdAfx.h"
#include "IInstanceEffectController.h"
#include "src/UserInterface/Core/CombatEvents.h"
#include "src/UserInterface/Core/EventBus.h"
#include "src/EterBase/ModernLogger.h"
#include "src/EterBase/Result.h"
#include "src/EterBase/StrongTypes.h"

#include <cstdint>
#include <string_view>
#include <memory>

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Zdarzenie emitowane po utworzeniu efektu trafienia skillem.
     * Decouples effect generation from GUI rendering.
     */
    struct SkillHitEffectCreatedEvent : public Core::IEvent
    {
        uint32_t effectHandle;
        EterBase::EntityId targetId;
        std::string_view boneName;

        SkillHitEffectCreatedEvent(uint32_t handle, EterBase::EntityId target, std::string_view bone)
            : effectHandle(handle), targetId(target), boneName(bone) {}
    };

    /**
     * @brief Izolowana klasa obslugujaca efekty trafienia skillem, bez monolitu.
     */
    class InstanceEffect_SkillHit : public IInstanceEffectController
    {
    public:
        explicit InstanceEffect_SkillHit(EterBase::EntityId ownerId)
            : m_ownerId(ownerId)
        {
            m_subscriptionId = Core::EventBus::GetInstance().Subscribe<Core::CombatEvents::ActorDamaged>(
                [this](const Core::CombatEvents::ActorDamaged& event) {
                    OnActorDamaged(event);
                });
        }

        ~InstanceEffect_SkillHit() override
        {
            if (m_subscriptionId != 0)
            {
                Core::EventBus::GetInstance().Unsubscribe<Core::CombatEvents::ActorDamaged>(m_subscriptionId);
            }
        }

        EterBase::PacketResult<uint32_t> AttachBoneEffect(const EffectAttachData& data) override
        {
            EterBase::ModernLogger::Info("Attaching skill hit effect. ID: {}, Bone: {}, Scale: {}", 
                                         data.effectId, data.boneName, data.scale);
            
            uint32_t simulatedHandle = 1; // Simulated handle for the created effect
            return simulatedHandle;
        }

        EterBase::PacketResult<void> DetachEffect(uint32_t handle) override
        {
            return EterBase::PacketResult<void>{};
        }

        void SetSwordAura(bool active, uint32_t auraType) override {}
        void SetBuffVisual(uint32_t buffId, bool active) override {}
        void SetStatusEffect(uint8_t statusFlag, bool active) override {}
        void SetItemShine(uint8_t partIndex, uint8_t refineLevel) override {}
        void ClearAllEffects() override {}

    private:
        void OnActorDamaged(const Core::CombatEvents::ActorDamaged& event)
        {
            if (event.victimId != m_ownerId)
            {
                return; // Filter to avoid global effect duplication
            }

            if (event.usedSkill.has_value())
            {
                EterBase::ModernLogger::Debug("Skill hit detected for instance {}. Attacker: {}", 
                                              m_ownerId.value(), event.attackerId.value());

                EffectAttachData attachData;
                attachData.effectId = 100; // Example effect ID for skill hit
                attachData.boneName = "Bip01 Spine2";
                attachData.scale = 1.0f;
                attachData.isLooping = false;

                auto result = AttachBoneEffect(attachData);
                if (result.has_value())
                {
                    Core::EventBus::GetInstance().Publish(
                        SkillHitEffectCreatedEvent{result.value(), event.victimId, attachData.boneName}
                    );
                }
            }
        }

        EterBase::EntityId m_ownerId;
        uint32_t m_subscriptionId{0};
    };

} // namespace UserInterface::InstanceControllers

extern "C" UserInterface::InstanceControllers::IInstanceEffectController* CreateSkillHitEffectController(EterBase::EntityId ownerId)
{
    return new UserInterface::InstanceControllers::InstanceEffect_SkillHit(ownerId);
}
