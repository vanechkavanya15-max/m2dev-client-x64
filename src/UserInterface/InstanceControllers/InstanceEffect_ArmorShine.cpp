#include "../StdAfx.h"
#include "IInstanceEffectController.h"
#include "../Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include <memory>

namespace
{
    struct ArmorShineSetEvent : public UserInterface::Core::IEvent
    {
        EterBase::ItemSlot partIndex;
        uint8_t refineLevel;

        ArmorShineSetEvent(EterBase::ItemSlot partIndex_, uint8_t refineLevel_)
            : partIndex(partIndex_), refineLevel(refineLevel_)
        {
        }
    };
    
    struct ArmorShineClearEvent : public UserInterface::Core::IEvent
    {
    };
}

namespace UserInterface::InstanceControllers
{
    class InstanceEffect_ArmorShine : public IInstanceEffectController
    {
    public:
        virtual ~InstanceEffect_ArmorShine() = default;

        EterBase::PacketResult<uint32_t> AttachBoneEffect(const EffectAttachData& data) override
        {
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }

        EterBase::PacketResult<void> DetachEffect(uint32_t handle) override
        {
            return {};
        }

        void SetSwordAura(bool active, uint32_t auraType) override
        {
        }

        void SetBuffVisual(uint32_t buffId, bool active) override
        {
        }

        void SetStatusEffect(uint8_t statusFlag, bool active) override
        {
        }

        void SetItemShine(uint8_t partIndexRaw, uint8_t refineLevel) override
        {
            EterBase::ItemSlot partIndex{partIndexRaw};
            
            if (refineLevel < 7) {
                ClearAllEffects();
                return;
            }
            
            EterBase::ModernLogger::Info("ArmorShine: Setting part {} to refine level {}", partIndex.value(), refineLevel);
            UserInterface::Core::EventBus::GetInstance().Publish(ArmorShineSetEvent{partIndex, refineLevel});
        }

        void ClearAllEffects() override
        {
            EterBase::ModernLogger::Debug("ArmorShine: Cleared all effects");
            UserInterface::Core::EventBus::GetInstance().Publish(ArmorShineClearEvent{});
        }
    };
    
    std::unique_ptr<IInstanceEffectController> CreateArmorShineController()
    {
        return std::make_unique<InstanceEffect_ArmorShine>();
    }
}
