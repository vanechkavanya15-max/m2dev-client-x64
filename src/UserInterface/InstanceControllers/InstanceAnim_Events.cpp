#include "../StdAfx.h"
#include "IInstanceAnimationController.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"

namespace UserInterface::InstanceControllers {

    class InstanceAnimationControllerEventsImpl final : public IInstanceAnimationController {
    public:
        explicit InstanceAnimationControllerEventsImpl(EterBase::EntityId entityId)
            : m_entityId(entityId) {}

        // --- Event Emission Methods ---
        void OnHitFrame(uint8_t hitIndex) {
            Core::EventBus::GetInstance().Publish(Core::AnimHitFrameEvent(m_entityId, m_currentMotionKey, hitIndex));
            EterBase::ModernLogger::Debug("Entity {} emitted AnimHitFrameEvent (Key: {}, HitIndex: {})", 
                m_entityId.value(), m_currentMotionKey, hitIndex);
        }

        void OnAnimationFinished() {
            Core::EventBus::GetInstance().Publish(Core::AnimFinishedEvent(m_entityId, m_currentMotionKey));
            EterBase::ModernLogger::Debug("Entity {} emitted AnimFinishedEvent (Key: {})", 
                m_entityId.value(), m_currentMotionKey);
        }

        // --- IInstanceAnimationController Implementation ---
        EterBase::PacketResult<void> PlayMotion(const MotionConfig& config) override {
            m_currentMotionKey = config.motionKey;
            m_state = MotionState::Skill; 
            EterBase::ModernLogger::Trace("Entity {} playing motion {}", m_entityId.value(), m_currentMotionKey);
            return {};
        }

        EterBase::PacketResult<void> BlendMotion(uint32_t motionKey, float blendDuration) override {
            m_currentMotionKey = motionKey;
            return {};
        }

        void SetMotionSpeed(float multiplier) override {
            m_speed = multiplier;
        }

        bool IsMotionFinished() const override { 
            return m_state == MotionState::Idle; 
        }

        void CancelMotion() override { 
            m_currentMotionKey = 0; 
            m_state = MotionState::Idle; 
        }

        uint32_t GetCurrentMotion() const override { 
            return m_currentMotionKey; 
        }

        MotionState GetMotionState() const override { 
            return m_state; 
        }

        void Clear() override { 
            m_currentMotionKey = 0; 
            m_state = MotionState::Idle; 
            m_speed = 1.0f;
        }

    private:
        EterBase::EntityId m_entityId;
        uint32_t m_currentMotionKey{0};
        MotionState m_state{MotionState::Idle};
        float m_speed{1.0f};
    };

} // namespace UserInterface::InstanceControllers

namespace UserInterface::InstanceControllers {
    std::unique_ptr<IInstanceAnimationController> CreateInstanceAnimationController(EterBase::EntityId entityId) {
        return std::make_unique<InstanceAnimationControllerEventsImpl>(entityId);
    }
}
