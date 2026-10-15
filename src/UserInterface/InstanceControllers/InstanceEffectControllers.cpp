#include "../StdAfx.h"
#include "InstanceEffectControllers.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::InstanceControllers
{
    // =========================================================================
    // InstanceEffect_ArmorShine
    // =========================================================================

    EterBase::PacketResult<uint32_t> InstanceEffect_ArmorShine::AttachBoneEffect(const EffectAttachData& /*data*/)
    {
        return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
    }

    EterBase::PacketResult<void> InstanceEffect_ArmorShine::DetachEffect(uint32_t /*handle*/)
    {
        return {};
    }

    void InstanceEffect_ArmorShine::SetSwordAura(bool /*active*/, uint32_t /*auraType*/)
    {
    }

    void InstanceEffect_ArmorShine::SetBuffVisual(uint32_t /*buffId*/, bool /*active*/)
    {
    }

    void InstanceEffect_ArmorShine::SetStatusEffect(uint8_t /*statusFlag*/, bool /*active*/)
    {
    }

    void InstanceEffect_ArmorShine::SetItemShine(uint8_t partIndexRaw, uint8_t refineLevel)
    {
        EterBase::ItemSlot partIndex{partIndexRaw};

        if (refineLevel < 7)
        {
            ClearAllEffects();
            return;
        }

        EterBase::ModernLogger::Info("ArmorShine: Setting part {} to refine level {}", partIndex.value(), refineLevel);
        UserInterface::Core::EventBus::GetInstance().Publish(ArmorShineSetEvent{partIndex, refineLevel});
    }

    void InstanceEffect_ArmorShine::ClearAllEffects()
    {
        EterBase::ModernLogger::Debug("ArmorShine: Cleared all effects");
        UserInterface::Core::EventBus::GetInstance().Publish(ArmorShineClearEvent{});
    }

    // =========================================================================
    // InstanceEffect_Aura
    // =========================================================================

    InstanceEffect_Aura::~InstanceEffect_Aura()
    {
        ClearAllEffects();
    }

    EterBase::PacketResult<uint32_t> InstanceEffect_Aura::AttachBoneEffect(const EffectAttachData& data)
    {
        EterBase::ModernLogger::Info("AttachBoneEffect: effectId={}, bone='{}', scale={}",
            data.effectId, data.boneName, data.scale);

        uint32_t handle = ++nextHandle;
        activeEffects.emplace(handle, EterBase::SkillId(data.effectId));

        Core::EventBus::GetInstance().Publish(BoneEffectAttachedEvent{handle, EterBase::SkillId(data.effectId)});
        return handle;
    }

    EterBase::PacketResult<void> InstanceEffect_Aura::DetachEffect(uint32_t handle)
    {
        if (auto it = activeEffects.find(handle); it != activeEffects.end())
        {
            EterBase::ModernLogger::Info("DetachEffect: Detaching handle {}", handle);
            activeEffects.erase(it);

            Core::EventBus::GetInstance().Publish(BoneEffectDetachedEvent{handle});
            return {};
        }

        EterBase::ModernLogger::Error("DetachEffect: Handle {} not found", handle);
        return std::unexpected(EterBase::PacketError::UnknownOpcode);
    }

    void InstanceEffect_Aura::SetSwordAura(bool active, uint32_t auraType)
    {
        EterBase::ModernLogger::Info("SetSwordAura: active={}, auraType={}", active, auraType);

        if (active)
        {
            if (auraEffectHandle != 0)
            {
                if (currentAuraType == auraType)
                {
                    return;
                }
                auto _ = DetachEffect(auraEffectHandle);
            }

            EffectAttachData attachData{
                .effectId = auraType,
                .boneName = "Bip01 Spine2",
                .scale = 1.0f,
                .isLooping = true
            };

            auto result = AttachBoneEffect(attachData);
            if (result.has_value())
            {
                auraEffectHandle = result.value();
                currentAuraType = auraType;
                Core::EventBus::GetInstance().Publish(AuraStateChangedEvent{true, EterBase::SkillId(auraType)});
            }
        }
        else
        {
            if (auraEffectHandle != 0)
            {
                auto _ = DetachEffect(auraEffectHandle);
                Core::EventBus::GetInstance().Publish(AuraStateChangedEvent{false, EterBase::SkillId(currentAuraType)});
                auraEffectHandle = 0;
                currentAuraType = 0;
            }
        }
    }

    void InstanceEffect_Aura::SetBuffVisual(uint32_t buffId, bool active)
    {
        EterBase::ModernLogger::Info("SetBuffVisual: buffId={}, active={}", buffId, active);
    }

    void InstanceEffect_Aura::SetStatusEffect(uint8_t statusFlag, bool active)
    {
        EterBase::ModernLogger::Info("SetStatusEffect: statusFlag={}, active={}", statusFlag, active);
    }

    void InstanceEffect_Aura::SetItemShine(uint8_t partIndex, uint8_t refineLevel)
    {
        EterBase::ModernLogger::Info("SetItemShine: partIndex={}, refineLevel={}", partIndex, refineLevel);
    }

    void InstanceEffect_Aura::ClearAllEffects()
    {
        EterBase::ModernLogger::Info("ClearAllEffects: Clearing all visual effects.");

        std::vector<uint32_t> handles;
        handles.reserve(activeEffects.size());
        for (const auto& [handle, _] : activeEffects)
        {
            handles.push_back(handle);
        }

        for (auto handle : handles)
        {
            auto _ = DetachEffect(handle);
        }

        auraEffectHandle = 0;
        currentAuraType = 0;
    }

    // =========================================================================
    // InstanceEffect_Bone
    // =========================================================================

    EterBase::PacketResult<uint32_t> InstanceEffect_Bone::AttachBoneEffect(const EffectAttachData& data)
    {
        if (data.effectId == 0)
        {
            EterBase::ModernLogger::Warning("Attempted to attach bone effect with invalid effectId.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        if (data.boneName.empty())
        {
            EterBase::ModernLogger::Warning("Attempted to attach bone effect with empty boneName.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        uint32_t handle = ++m_nextHandle;

        m_activeEffects.emplace(handle, data.effectId);
        EterBase::ModernLogger::Info("Attached bone effect {} to bone {} with handle {}.", data.effectId, data.boneName, handle);

        Core::EventBus::GetInstance().Publish(EffectAttachedEvent(handle, data.effectId));
        return handle;
    }

    EterBase::PacketResult<void> InstanceEffect_Bone::DetachEffect(uint32_t handle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_activeEffects.find(handle);
        if (it == m_activeEffects.end())
        {
            EterBase::ModernLogger::Warning("Attempted to detach non-existent effect handle {}.", handle);
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        m_activeEffects.erase(it);
        EterBase::ModernLogger::Info("Detached effect handle {}.", handle);

        Core::EventBus::GetInstance().Publish(EffectDetachedEvent(handle));
        return {};
    }

    void InstanceEffect_Bone::SetSwordAura(bool active, uint32_t auraType)
    {
        EterBase::ModernLogger::Info("SetSwordAura: active={}, auraType={}", active, auraType);
    }

    void InstanceEffect_Bone::SetBuffVisual(uint32_t buffId, bool active)
    {
        EterBase::ModernLogger::Info("SetBuffVisual: buffId={}, active={}", buffId, active);
    }

    void InstanceEffect_Bone::SetStatusEffect(uint8_t statusFlag, bool active)
    {
        EterBase::ModernLogger::Info("SetStatusEffect: statusFlag={}, active={}", statusFlag, active);
    }

    void InstanceEffect_Bone::SetItemShine(uint8_t partIndex, uint8_t refineLevel)
    {
        EterBase::ModernLogger::Info("SetItemShine: partIndex={}, refineLevel={}", partIndex, refineLevel);
    }

    void InstanceEffect_Bone::ClearAllEffects()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (const auto& [handle, _] : m_activeEffects)
        {
            Core::EventBus::GetInstance().Publish(EffectDetachedEvent(handle));
        }

        m_activeEffects.clear();
        EterBase::ModernLogger::Info("Cleared all active effects.");
    }

    // =========================================================================
    // InstanceEffect_BuffVisual
    // =========================================================================

    EterBase::PacketResult<uint32_t> InstanceEffect_BuffVisual::AttachBoneEffect(const EffectAttachData& data)
    {
        EterBase::ModernLogger::Info("AttachBoneEffect called for effectId: {}, boneName: {}", data.effectId, data.boneName);

        uint32_t handle = ++nextEffectHandle_;
        activeEffects_[handle] = data.effectId;
        return handle;
    }

    EterBase::PacketResult<void> InstanceEffect_BuffVisual::DetachEffect(uint32_t handle)
    {
        if (activeEffects_.erase(handle) > 0)
        {
            EterBase::ModernLogger::Info("DetachEffect: Effect detached successfully (handle: {})", handle);
            return {};
        }

        EterBase::ModernLogger::Warn("DetachEffect: Invalid handle ({})", handle);
        return EterBase::MakeError(EterBase::PacketError::None);
    }

    void InstanceEffect_BuffVisual::SetSwordAura(bool active, uint32_t auraType)
    {
        EterBase::ModernLogger::Info("SetSwordAura: active={}, auraType={}", active, auraType);
        Core::EventBus::GetInstance().Publish(SwordAuraChangedEvent{active, auraType});
    }

    void InstanceEffect_BuffVisual::SetBuffVisual(uint32_t buffId, bool active)
    {
        EterBase::ModernLogger::Info("SetBuffVisual: buffId={}, active={}", buffId, active);

        if (active)
        {
            activeBuffs_[buffId] = true;
        }
        else
        {
            activeBuffs_.erase(buffId);
        }

        Core::EventBus::GetInstance().Publish(BuffVisualStateChangedEvent{buffId, active});
    }

    void InstanceEffect_BuffVisual::SetStatusEffect(uint8_t statusFlag, bool active)
    {
        EterBase::ModernLogger::Info("SetStatusEffect: statusFlag={}, active={}", statusFlag, active);
        Core::EventBus::GetInstance().Publish(StatusEffectChangedEvent{statusFlag, active});
    }

    void InstanceEffect_BuffVisual::SetItemShine(uint8_t partIndex, uint8_t refineLevel)
    {
        EterBase::ModernLogger::Info("SetItemShine: partIndex={}, refineLevel={}", partIndex, refineLevel);
        Core::EventBus::GetInstance().Publish(ItemShineChangedEvent{partIndex, refineLevel});
    }

    void InstanceEffect_BuffVisual::ClearAllEffects()
    {
        EterBase::ModernLogger::Info("ClearAllEffects: Clearing all buffs and effects.");

        for (const auto& [buffId, isActive] : activeBuffs_)
        {
            Core::EventBus::GetInstance().Publish(BuffVisualStateChangedEvent{buffId, false});
        }

        activeBuffs_.clear();
        activeEffects_.clear();
    }

    // =========================================================================
    // InstanceEffect_PoisonFire
    // =========================================================================

    constexpr uint8_t STATUS_POISON = 1 << 0;
    constexpr uint8_t STATUS_FIRE = 1 << 1;
    constexpr uint8_t STATUS_BLEEDING = 1 << 2;
    constexpr uint8_t STATUS_FREEZING = 1 << 3;

    void InstanceEffect_PoisonFire::HandleEffectChange(uint8_t flag, bool& currentState, bool newState, std::string_view effectName)
    {
        if (currentState != newState)
        {
            currentState = newState;
            if (newState)
            {
                EterBase::ModernLogger::Info("InstanceEffect_PoisonFire: {} effect started.", effectName);
            }
            else
            {
                EterBase::ModernLogger::Info("InstanceEffect_PoisonFire: {} effect stopped.", effectName);
            }

            UserInterface::Core::EventBus::GetInstance().Publish(StatusEffectVisualEvent{flag, newState});
        }
    }

    EterBase::PacketResult<uint32_t> InstanceEffect_PoisonFire::AttachBoneEffect(const EffectAttachData& data)
    {
        EterBase::ModernLogger::Debug("InstanceEffect_PoisonFire: AttachBoneEffect ID: {}", data.effectId);
        return 1;
    }

    EterBase::PacketResult<void> InstanceEffect_PoisonFire::DetachEffect(uint32_t handle)
    {
        EterBase::ModernLogger::Debug("InstanceEffect_PoisonFire: DetachEffect handle: {}", handle);
        return {};
    }

    void InstanceEffect_PoisonFire::SetSwordAura(bool active, uint32_t auraType)
    {
        EterBase::ModernLogger::Debug("InstanceEffect_PoisonFire: SetSwordAura active: {}, type: {}", active, auraType);
    }

    void InstanceEffect_PoisonFire::SetBuffVisual(uint32_t buffId, bool active)
    {
        EterBase::ModernLogger::Debug("InstanceEffect_PoisonFire: SetBuffVisual ID: {}, active: {}", buffId, active);
    }

    void InstanceEffect_PoisonFire::SetStatusEffect(uint8_t statusFlag, bool active)
    {
        EterBase::ModernLogger::Debug("InstanceEffect_PoisonFire: SetStatusEffect flag: {}, active: {}", statusFlag, active);

        if (statusFlag & STATUS_POISON)
        {
            HandleEffectChange(STATUS_POISON, m_isPoisoned, active, "Poison");
        }
        if (statusFlag & STATUS_FIRE)
        {
            HandleEffectChange(STATUS_FIRE, m_isAflame, active, "Fire");
        }
        if (statusFlag & STATUS_BLEEDING)
        {
            HandleEffectChange(STATUS_BLEEDING, m_isBleeding, active, "Bleeding");
        }
        if (statusFlag & STATUS_FREEZING)
        {
            HandleEffectChange(STATUS_FREEZING, m_isFreezing, active, "Freezing");
        }
    }

    void InstanceEffect_PoisonFire::SetItemShine(uint8_t partIndex, uint8_t refineLevel)
    {
        EterBase::ModernLogger::Debug("InstanceEffect_PoisonFire: SetItemShine part: {}, refine: {}", partIndex, refineLevel);
    }

    void InstanceEffect_PoisonFire::ClearAllEffects()
    {
        EterBase::ModernLogger::Info("InstanceEffect_PoisonFire: Clearing all effects.");

        if (m_isPoisoned) HandleEffectChange(STATUS_POISON, m_isPoisoned, false, "Poison");
        if (m_isAflame) HandleEffectChange(STATUS_FIRE, m_isAflame, false, "Fire");
        if (m_isBleeding) HandleEffectChange(STATUS_BLEEDING, m_isBleeding, false, "Bleeding");
        if (m_isFreezing) HandleEffectChange(STATUS_FREEZING, m_isFreezing, false, "Freezing");
    }

    // =========================================================================
    // InstanceEffect_SkillHit
    // =========================================================================

    InstanceEffect_SkillHit::InstanceEffect_SkillHit(EterBase::EntityId ownerId)
        : m_ownerId(ownerId)
    {
        m_subscriptionId = Core::EventBus::GetInstance().Subscribe<Core::CombatEvents::ActorDamaged>(
            [this](const Core::CombatEvents::ActorDamaged& event) {
                OnActorDamaged(event);
            });
    }

    InstanceEffect_SkillHit::~InstanceEffect_SkillHit()
    {
        if (m_subscriptionId != 0)
        {
            Core::EventBus::GetInstance().Unsubscribe<Core::CombatEvents::ActorDamaged>(m_subscriptionId);
        }
    }

    EterBase::PacketResult<uint32_t> InstanceEffect_SkillHit::AttachBoneEffect(const EffectAttachData& data)
    {
        EterBase::ModernLogger::Info("Attaching skill hit effect. ID: {}, Bone: {}, Scale: {}",
            data.effectId, data.boneName, data.scale);
        return 1;
    }

    EterBase::PacketResult<void> InstanceEffect_SkillHit::DetachEffect(uint32_t /*handle*/)
    {
        return {};
    }

    void InstanceEffect_SkillHit::SetSwordAura(bool /*active*/, uint32_t /*auraType*/) {}
    void InstanceEffect_SkillHit::SetBuffVisual(uint32_t /*buffId*/, bool /*active*/) {}
    void InstanceEffect_SkillHit::SetStatusEffect(uint8_t /*statusFlag*/, bool /*active*/) {}
    void InstanceEffect_SkillHit::SetItemShine(uint8_t /*partIndex*/, uint8_t /*refineLevel*/) {}
    void InstanceEffect_SkillHit::ClearAllEffects() {}

    void InstanceEffect_SkillHit::OnActorDamaged(const Core::CombatEvents::ActorDamaged& event)
    {
        if (event.victimId != m_ownerId)
        {
            return;
        }

        if (event.usedSkill.has_value())
        {
            EterBase::ModernLogger::Debug("Skill hit detected for instance {}. Attacker: {}",
                m_ownerId.value(), event.attackerId.value());

            EffectAttachData attachData;
            attachData.effectId = 100;
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

    // =========================================================================
    // InstanceEffectClearer
    // =========================================================================

    EterBase::PacketResult<void> InstanceEffectClearer::ClearAllEffects(IInstanceEffectController& controller, EterBase::EntityId entityId)
    {
        EterBase::ModernLogger::Debug("Zarzadano wyczyszczenia efektow dla instancji (EntityId: {})", entityId.value());

        controller.ClearAllEffects();
        Core::EventBus::GetInstance().Publish(EffectClearedEvent{entityId});

        EterBase::ModernLogger::Info("Pomyslnie odpieto i wyczyszczono efekty (EntityId: {})", entityId.value());
        return {};
    }

    // =========================================================================
    // Funkcje pomocnicze i fabryki
    // =========================================================================

    EterBase::PacketResult<void> UpdateWeaponGlow(
        IInstanceEffectController& effectController,
        EterBase::EntityId entityId,
        uint8_t partIndex,
        uint8_t refineLevel,
        uint32_t shineData)
    {
        EterBase::ModernLogger::Info(
            "UpdateWeaponGlow: Entity: {}, Part: {}, Refine: +{}, ShineData: {}",
            entityId.value(), partIndex, refineLevel, shineData
        );

        if (entityId.value() == 0)
        {
            EterBase::ModernLogger::Error("UpdateWeaponGlow: Niewazne ID encji.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        if (refineLevel > 0 && refineLevel < 7)
        {
            EterBase::ModernLogger::Debug(
                "UpdateWeaponGlow: Poziom ulepszenia {} nie aktywuje efektu swiecenia (+7 do +9).",
                refineLevel
            );
            return {};
        }

        effectController.SetItemShine(partIndex, refineLevel);

        UserInterface::Core::EventBus::GetInstance().Publish(
            WeaponGlowUpdateEvent{entityId, partIndex, refineLevel, shineData}
        );

        return {};
    }

    std::unique_ptr<IInstanceEffectController> CreateInstanceEffectController()
    {
        return std::make_unique<InstanceEffect_Aura>();
    }

    std::unique_ptr<IInstanceEffectController> CreateArmorShineController()
    {
        return std::make_unique<InstanceEffect_ArmorShine>();
    }

    std::unique_ptr<IInstanceEffectController> CreateBuffVisualController()
    {
        return std::make_unique<InstanceEffect_BuffVisual>();
    }

    std::unique_ptr<IInstanceEffectController> CreatePoisonFireEffectController()
    {
        return std::make_unique<InstanceEffect_PoisonFire>();
    }
}

extern "C" UserInterface::InstanceControllers::IInstanceEffectController* CreateSkillHitEffectController(EterBase::EntityId ownerId)
{
    return new UserInterface::InstanceControllers::InstanceEffect_SkillHit(ownerId);
}
