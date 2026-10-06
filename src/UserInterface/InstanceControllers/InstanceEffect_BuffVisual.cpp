#include "../StdAfx.h"
#include "IInstanceEffectController.h"
#include "EterBase/LogModern.h"
#include <format>
#include "UserInterface/Core/EventBus.h"
#include <unordered_map>
#include <memory>
#include <string>

namespace UserInterface::InstanceControllers
{
    // Custom events inside the .cpp to adhere to Zero-Conflict rule
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

    class InstanceEffect_BuffVisual : public IInstanceEffectController
    {
    public:
        InstanceEffect_BuffVisual() = default;
        ~InstanceEffect_BuffVisual() override = default;

        EterBase::PacketResult<uint32_t> AttachBoneEffect(const EffectAttachData& data) override
        {
            EterBase::ModernLogger::Info("AttachBoneEffect called for effectId: {}, boneName: {}", data.effectId, data.boneName);
            
            uint32_t handle = ++nextEffectHandle_;
            activeEffects_[handle] = data.effectId;
            return handle;
        }

        EterBase::PacketResult<void> DetachEffect(uint32_t handle) override
        {
            if (activeEffects_.erase(handle) > 0) {
                EterBase::ModernLogger::Info("DetachEffect: Effect detached successfully (handle: {})", handle);
                return {};
            }
            
            EterBase::ModernLogger::Warn("DetachEffect: Invalid handle ({})", handle);
            return EterBase::MakeError(EterBase::PacketError::None);
        }

        void SetSwordAura(bool active, uint32_t auraType) override
        {
            EterBase::ModernLogger::Info("SetSwordAura: active={}, auraType={}", active, auraType);
            Core::EventBus::GetInstance().Publish(SwordAuraChangedEvent{active, auraType});
        }

        void SetBuffVisual(uint32_t buffId, bool active) override
        {
            EterBase::ModernLogger::Info("SetBuffVisual: buffId={}, active={}", buffId, active);
            
            if (active) {
                activeBuffs_[buffId] = true;
            } else {
                activeBuffs_.erase(buffId);
            }
            
            Core::EventBus::GetInstance().Publish(BuffVisualStateChangedEvent{buffId, active});
        }

        void SetStatusEffect(uint8_t statusFlag, bool active) override
        {
            EterBase::ModernLogger::Info("SetStatusEffect: statusFlag={}, active={}", statusFlag, active);
            Core::EventBus::GetInstance().Publish(StatusEffectChangedEvent{statusFlag, active});
        }

        void SetItemShine(uint8_t partIndex, uint8_t refineLevel) override
        {
            EterBase::ModernLogger::Info("SetItemShine: partIndex={}, refineLevel={}", partIndex, refineLevel);
            Core::EventBus::GetInstance().Publish(ItemShineChangedEvent{partIndex, refineLevel});
        }

        void ClearAllEffects() override
        {
            EterBase::ModernLogger::Info("ClearAllEffects: Clearing all buffs and effects.");
            
            for (const auto& [buffId, isActive] : activeBuffs_) {
                Core::EventBus::GetInstance().Publish(BuffVisualStateChangedEvent{buffId, false});
            }
            
            activeBuffs_.clear();
            activeEffects_.clear();
        }

    private:
        uint32_t nextEffectHandle_{0};
        std::unordered_map<uint32_t, uint32_t> activeEffects_; // handle -> effectId
        std::unordered_map<uint32_t, bool> activeBuffs_;
    };

    std::unique_ptr<IInstanceEffectController> CreateBuffVisualController()
    {
        return std::make_unique<InstanceEffect_BuffVisual>();
    }
}
