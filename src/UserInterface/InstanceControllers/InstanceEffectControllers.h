#pragma once

#include <cstdint>
#include <string_view>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "../Core/EventBus.h"
#include "../Core/CombatEvents.h"

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Dane potrzebne do przyczepienia efektu do kosci szkieletu modelu.
     */
    struct EffectAttachData
    {
        uint32_t effectId{0};
        std::string_view boneName;
        float scale{1.0f};
        bool isLooping{true};
    };

    // --- Zdarzenia efektow instancji ---

    struct ArmorShineSetEvent : public Core::IEvent
    {
        EterBase::ItemSlot partIndex;
        uint8_t refineLevel;

        ArmorShineSetEvent(EterBase::ItemSlot partIndex_, uint8_t refineLevel_)
            : partIndex(partIndex_), refineLevel(refineLevel_) {}
    };

    struct ArmorShineClearEvent : public Core::IEvent {};

    struct BoneEffectAttachedEvent : public Core::IEvent
    {
        uint32_t handle;
        EterBase::SkillId effectId;

        BoneEffectAttachedEvent(uint32_t h, EterBase::SkillId id) : handle(h), effectId(id) {}
    };

    struct BoneEffectDetachedEvent : public Core::IEvent
    {
        uint32_t handle;

        explicit BoneEffectDetachedEvent(uint32_t h) : handle(h) {}
    };

    struct AuraStateChangedEvent : public Core::IEvent
    {
        bool active;
        EterBase::SkillId auraType;

        AuraStateChangedEvent(bool act, EterBase::SkillId type) : active(act), auraType(type) {}
    };

    struct EffectAttachedEvent : public Core::IEvent
    {
        uint32_t handle;
        uint32_t effectId;

        EffectAttachedEvent(uint32_t h, uint32_t id) : handle(h), effectId(id) {}
    };

    struct EffectDetachedEvent : public Core::IEvent
    {
        uint32_t handle;

        explicit EffectDetachedEvent(uint32_t h) : handle(h) {}
    };

    struct BuffVisualStateChangedEvent : public Core::IEvent
    {
        uint32_t buffId;
        bool isActive;

        BuffVisualStateChangedEvent(uint32_t buffId, bool isActive)
            : buffId(buffId), isActive(isActive) {}
    };

    struct StatusEffectChangedEvent : public Core::IEvent
    {
        uint8_t statusFlag;
        bool isActive;

        StatusEffectChangedEvent(uint8_t statusFlag, bool isActive)
            : statusFlag(statusFlag), isActive(isActive) {}
    };

    struct StatusEffectVisualEvent : public Core::IEvent
    {
        uint8_t flag;
        bool active;

        StatusEffectVisualEvent(uint8_t f, bool a) : flag(f), active(a) {}
    };

    struct SwordAuraChangedEvent : public Core::IEvent
    {
        bool isActive;
        uint32_t auraType;

        SwordAuraChangedEvent(bool isActive, uint32_t auraType)
            : isActive(isActive), auraType(auraType) {}
    };

    struct ItemShineChangedEvent : public Core::IEvent
    {
        uint8_t partIndex;
        uint8_t refineLevel;

        ItemShineChangedEvent(uint8_t partIndex, uint8_t refineLevel)
            : partIndex(partIndex), refineLevel(refineLevel) {}
    };

    struct EffectClearedEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;

        explicit EffectClearedEvent(EterBase::EntityId id) : entityId(id) {}
    };

    struct SkillHitEffectCreatedEvent : public Core::IEvent
    {
        uint32_t effectHandle;
        EterBase::EntityId targetId;
        std::string_view boneName;

        SkillHitEffectCreatedEvent(uint32_t handle, EterBase::EntityId target, std::string_view bone)
            : effectHandle(handle), targetId(target), boneName(bone) {}
    };

    struct WeaponGlowUpdateEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        uint8_t partIndex;
        uint8_t refineLevel;
        uint32_t shineData;

        WeaponGlowUpdateEvent(EterBase::EntityId id, uint8_t part, uint8_t refine, uint32_t shine)
            : entityId(id), partIndex(part), refineLevel(refine), shineData(shine) {}
    };

    /**
     * @brief Glowny interfejs kontrolera efektow wizualnych instancji.
     */
    class IInstanceEffectController
    {
    public:
        virtual ~IInstanceEffectController() = default;

        virtual EterBase::PacketResult<uint32_t> AttachBoneEffect(const EffectAttachData& data) = 0;
        virtual EterBase::PacketResult<void> DetachEffect(uint32_t handle) = 0;
        virtual void SetSwordAura(bool active, uint32_t auraType) = 0;
        virtual void SetBuffVisual(uint32_t buffId, bool active) = 0;
        virtual void SetStatusEffect(uint8_t statusFlag, bool active) = 0;
        virtual void SetItemShine(uint8_t partIndex, uint8_t refineLevel) = 0;
        virtual void ClearAllEffects() = 0;
    };

    /**
     * @brief Kontroler efektow blysku pancerza (+7, +8, +9).
     */
    class InstanceEffect_ArmorShine : public IInstanceEffectController
    {
    public:
        ~InstanceEffect_ArmorShine() override = default;

        EterBase::PacketResult<uint32_t> AttachBoneEffect(const EffectAttachData& data) override;
        EterBase::PacketResult<void> DetachEffect(uint32_t handle) override;
        void SetSwordAura(bool active, uint32_t auraType) override;
        void SetBuffVisual(uint32_t buffId, bool active) override;
        void SetStatusEffect(uint8_t statusFlag, bool active) override;
        void SetItemShine(uint8_t partIndexRaw, uint8_t refineLevel) override;
        void ClearAllEffects() override;
    };

    /**
     * @brief Kontroler efektow aury miecza i ciala.
     */
    class InstanceEffect_Aura final : public IInstanceEffectController
    {
    public:
        InstanceEffect_Aura() = default;
        ~InstanceEffect_Aura() override;

        EterBase::PacketResult<uint32_t> AttachBoneEffect(const EffectAttachData& data) override;
        EterBase::PacketResult<void> DetachEffect(uint32_t handle) override;
        void SetSwordAura(bool active, uint32_t auraType) override;
        void SetBuffVisual(uint32_t buffId, bool active) override;
        void SetStatusEffect(uint8_t statusFlag, bool active) override;
        void SetItemShine(uint8_t partIndex, uint8_t refineLevel) override;
        void ClearAllEffects() override;

    private:
        uint32_t nextHandle{0};
        std::unordered_map<uint32_t, EterBase::SkillId> activeEffects;
        uint32_t auraEffectHandle{0};
        uint32_t currentAuraType{0};
    };

    /**
     * @brief Kontroler podpinania efektow bezposrednio pod wezly kosci modelu.
     */
    class InstanceEffect_Bone : public IInstanceEffectController
    {
    public:
        InstanceEffect_Bone() = default;
        ~InstanceEffect_Bone() override = default;

        EterBase::PacketResult<uint32_t> AttachBoneEffect(const EffectAttachData& data) override;
        EterBase::PacketResult<void> DetachEffect(uint32_t handle) override;
        void SetSwordAura(bool active, uint32_t auraType) override;
        void SetBuffVisual(uint32_t buffId, bool active) override;
        void SetStatusEffect(uint8_t statusFlag, bool active) override;
        void SetItemShine(uint8_t partIndex, uint8_t refineLevel) override;
        void ClearAllEffects() override;

    private:
        std::mutex m_mutex;
        uint32_t m_nextHandle{0};
        std::unordered_map<uint32_t, uint32_t> m_activeEffects;
    };

    /**
     * @brief Kontroler wizualizacji aktywnych buffow i flag stanow.
     */
    class InstanceEffect_BuffVisual : public IInstanceEffectController
    {
    public:
        InstanceEffect_BuffVisual() = default;
        ~InstanceEffect_BuffVisual() override = default;

        EterBase::PacketResult<uint32_t> AttachBoneEffect(const EffectAttachData& data) override;
        EterBase::PacketResult<void> DetachEffect(uint32_t handle) override;
        void SetSwordAura(bool active, uint32_t auraType) override;
        void SetBuffVisual(uint32_t buffId, bool active) override;
        void SetStatusEffect(uint8_t statusFlag, bool active) override;
        void SetItemShine(uint8_t partIndex, uint8_t refineLevel) override;
        void ClearAllEffects() override;

    private:
        uint32_t nextEffectHandle_{0};
        std::unordered_map<uint32_t, uint32_t> activeEffects_;
        std::unordered_map<uint32_t, bool> activeBuffs_;
    };

    /**
     * @brief Kontroler efektow trucizny, podpalenia, krwawienia i zamrozenia.
     */
    class InstanceEffect_PoisonFire final : public IInstanceEffectController
    {
    public:
        InstanceEffect_PoisonFire() = default;
        ~InstanceEffect_PoisonFire() override = default;

        EterBase::PacketResult<uint32_t> AttachBoneEffect(const EffectAttachData& data) override;
        EterBase::PacketResult<void> DetachEffect(uint32_t handle) override;
        void SetSwordAura(bool active, uint32_t auraType) override;
        void SetBuffVisual(uint32_t buffId, bool active) override;
        void SetStatusEffect(uint8_t statusFlag, bool active) override;
        void SetItemShine(uint8_t partIndex, uint8_t refineLevel) override;
        void ClearAllEffects() override;

    private:
        void HandleEffectChange(uint8_t flag, bool& currentState, bool newState, std::string_view effectName);

        bool m_isPoisoned{false};
        bool m_isAflame{false};
        bool m_isBleeding{false};
        bool m_isFreezing{false};
    };

    /**
     * @brief Kontroler generowania efektow uderzenia skillem po zdarzeniu uszkodzenia aktora.
     */
    class InstanceEffect_SkillHit : public IInstanceEffectController
    {
    public:
        explicit InstanceEffect_SkillHit(EterBase::EntityId ownerId);
        ~InstanceEffect_SkillHit() override;

        EterBase::PacketResult<uint32_t> AttachBoneEffect(const EffectAttachData& data) override;
        EterBase::PacketResult<void> DetachEffect(uint32_t handle) override;
        void SetSwordAura(bool active, uint32_t auraType) override;
        void SetBuffVisual(uint32_t buffId, bool active) override;
        void SetStatusEffect(uint8_t statusFlag, bool active) override;
        void SetItemShine(uint8_t partIndex, uint8_t refineLevel) override;
        void ClearAllEffects() override;

    private:
        void OnActorDamaged(const Core::CombatEvents::ActorDamaged& event);

        EterBase::EntityId m_ownerId{0};
        uint32_t m_subscriptionId{0};
    };

    /**
     * @brief Pomocnik do bezpiecznego odpinania i czyszczenia efektow danej instancji.
     */
    class InstanceEffectClearer
    {
    public:
        static EterBase::PacketResult<void> ClearAllEffects(IInstanceEffectController& controller, EterBase::EntityId entityId);
    };

    // --- Pomocnicze funkcje domenowe i fabryki ---
    EterBase::PacketResult<void> UpdateWeaponGlow(
        IInstanceEffectController& effectController,
        EterBase::EntityId entityId,
        uint8_t partIndex,
        uint8_t refineLevel,
        uint32_t shineData);

    std::unique_ptr<IInstanceEffectController> CreateInstanceEffectController();
    std::unique_ptr<IInstanceEffectController> CreateArmorShineController();
    std::unique_ptr<IInstanceEffectController> CreateBuffVisualController();
    std::unique_ptr<IInstanceEffectController> CreatePoisonFireEffectController();
}

extern "C" UserInterface::InstanceControllers::IInstanceEffectController* CreateSkillHitEffectController(EterBase::EntityId ownerId);
