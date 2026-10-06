#pragma once

#include <cstdint>
#include <string_view>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::InstanceControllers
{
    struct EffectAttachData
    {
        uint32_t effectId{0};
        std::string_view boneName;
        float scale{1.0f};
        bool isLooping{true};
    };

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
}
