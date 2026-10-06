#include "../StdAfx.h"
#include "IInstanceEffectController.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

#include <unordered_map>
#include <mutex>

namespace UserInterface::InstanceControllers
{
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

    class InstanceEffectController : public IInstanceEffectController
    {
    public:
        InstanceEffectController() = default;
        ~InstanceEffectController() override = default;

        EterBase::PacketResult<uint32_t> AttachBoneEffect(const EffectAttachData& data) override
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

            EterBase::ModernLogger::Info("Attached bone effect {0} to bone {1} with handle {2}.", data.effectId, data.boneName, handle);

            EffectAttachedEvent event(handle, data.effectId);
            Core::EventBus::GetInstance().Publish(event);

            return handle;
        }

        EterBase::PacketResult<void> DetachEffect(uint32_t handle) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto it = m_activeEffects.find(handle);
            if (it == m_activeEffects.end())
            {
                EterBase::ModernLogger::Warning("Attempted to detach non-existent effect handle {0}.", handle);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            m_activeEffects.erase(it);

            EterBase::ModernLogger::Info("Detached effect handle {0}.", handle);

            EffectDetachedEvent event(handle);
            Core::EventBus::GetInstance().Publish(event);

            return {};
        }

        void SetSwordAura(bool active, uint32_t auraType) override
        {
            EterBase::ModernLogger::Info("SetSwordAura: active={0}, auraType={1}", active, auraType);
        }

        void SetBuffVisual(uint32_t buffId, bool active) override
        {
            EterBase::ModernLogger::Info("SetBuffVisual: buffId={0}, active={1}", buffId, active);
        }

        void SetStatusEffect(uint8_t statusFlag, bool active) override
        {
            EterBase::ModernLogger::Info("SetStatusEffect: statusFlag={0}, active={1}", statusFlag, active);
        }

        void SetItemShine(uint8_t partIndex, uint8_t refineLevel) override
        {
            EterBase::ModernLogger::Info("SetItemShine: partIndex={0}, refineLevel={1}", partIndex, refineLevel);
        }

        void ClearAllEffects() override
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            for (const auto& [handle, _] : m_activeEffects)
            {
                EffectDetachedEvent event(handle);
                Core::EventBus::GetInstance().Publish(event);
            }

            m_activeEffects.clear();

            EterBase::ModernLogger::Info("Cleared all active effects.");
        }

    private:
        std::mutex m_mutex;
        uint32_t m_nextHandle{0};
        std::unordered_map<uint32_t, uint32_t> m_activeEffects;
    };
}
