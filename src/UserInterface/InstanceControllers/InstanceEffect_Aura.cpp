#include "../StdAfx.h"
#include "IInstanceEffectController.h"
#include "../Core/EventBus.h"
#include "EterBase/ModernLogger.h"
#include "EterBase/StrongTypes.h"
#include <unordered_map>
#include <memory>
#include <expected>
#include <vector>

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Zdarzenie rozglaszajace zamocowanie efektu do instancji (Bone Effect).
     */
    struct BoneEffectAttachedEvent : public Core::IEvent
    {
        uint32_t handle;
        EterBase::SkillId effectId;
        
        BoneEffectAttachedEvent(uint32_t h, EterBase::SkillId id) : handle(h), effectId(id) {}
    };

    /**
     * @brief Zdarzenie rozglaszajace odlaczenie efektu od instancji.
     */
    struct BoneEffectDetachedEvent : public Core::IEvent
    {
        uint32_t handle;
        
        explicit BoneEffectDetachedEvent(uint32_t h) : handle(h) {}
    };

    /**
     * @brief Zdarzenie zmiany aury.
     */
    struct AuraStateChangedEvent : public Core::IEvent
    {
        bool active;
        EterBase::SkillId auraType;
        
        AuraStateChangedEvent(bool act, EterBase::SkillId type) : active(act), auraType(type) {}
    };

    /**
     * @brief Controller for managing visual effects on character instances (e.g., Sword Aura, Strong Body).
     * 
     * Strictly adheres to the Single Responsibility Principle (SRP) by focusing
     * only on attaching/detaching effects and propagating events via EventBus.
     */
    class InstanceEffect_Aura final : public IInstanceEffectController
    {
    public:
        InstanceEffect_Aura() = default;
        
        ~InstanceEffect_Aura() override
        {
            ClearAllEffects();
        }

        EterBase::PacketResult<uint32_t> AttachBoneEffect(const EffectAttachData& data) override
        {
            EterBase::ModernLogger::Info("AttachBoneEffect: effectId={}, bone='{}', scale={}", 
                data.effectId, data.boneName, data.scale);
            
            uint32_t handle = ++nextHandle;
            activeEffects.emplace(handle, EterBase::SkillId(data.effectId));
            
            Core::EventBus::GetInstance().Publish(BoneEffectAttachedEvent{handle, EterBase::SkillId(data.effectId)});
            
            return handle;
        }

        EterBase::PacketResult<void> DetachEffect(uint32_t handle) override
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

        void SetSwordAura(bool active, uint32_t auraType) override
        {
            EterBase::ModernLogger::Info("SetSwordAura: active={}, auraType={}", active, auraType);
            
            if (active)
            {
                if (auraEffectHandle != 0)
                {
                    if (currentAuraType == auraType)
                    {
                        return; // Aura is already active and of the same type
                    }
                    auto _ = DetachEffect(auraEffectHandle);
                }
                
                // Sword Aura or Strong Body often attaches to the main spine or right hand.
                // We pass a generic bone here for simplicity.
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

        void SetBuffVisual(uint32_t buffId, bool active) override
        {
            EterBase::ModernLogger::Info("SetBuffVisual: buffId={}, active={}", buffId, active);
            // Placeholder for buff visualization logic (e.g., Blessing, Attack Up)
        }

        void SetStatusEffect(uint8_t statusFlag, bool active) override
        {
            EterBase::ModernLogger::Info("SetStatusEffect: statusFlag={}, active={}", statusFlag, active);
            // Placeholder for status effects like poison, stun, slow
        }

        void SetItemShine(uint8_t partIndex, uint8_t refineLevel) override
        {
            EterBase::ModernLogger::Info("SetItemShine: partIndex={}, refineLevel={}", partIndex, refineLevel);
            // Placeholder for weapon/armor shine visualization based on refine level
        }

        void ClearAllEffects() override
        {
            EterBase::ModernLogger::Info("ClearAllEffects: Clearing all visual effects.");
            
            // Collect handles safely avoiding iterator invalidation during erase/detach
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

    private:
        uint32_t nextHandle{0};
        std::unordered_map<uint32_t, EterBase::SkillId> activeEffects;
        
        uint32_t auraEffectHandle{0};
        uint32_t currentAuraType{0};
    };

    /**
     * @brief Factory for creating an instance of the effect controller.
     */
    std::unique_ptr<IInstanceEffectController> CreateInstanceEffectController()
    {
        return std::make_unique<InstanceEffect_Aura>();
    }
}
