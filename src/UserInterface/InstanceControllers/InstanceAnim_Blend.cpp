#include "../StdAfx.h"

#include "IInstanceAnimationController.h"
#include "EterBase/ModernLogger.h"
#include "UserInterface/Core/EventBus.h"
#include <memory>

namespace UserInterface::InstanceControllers
{
    namespace
    {
        struct AnimationPlayedEvent : public UserInterface::Core::IEvent {
            uint32_t motionKey;
            float speed;

            AnimationPlayedEvent(uint32_t key, float s) : motionKey(key), speed(s) {}
        };

        struct AnimationBlendStartedEvent : public UserInterface::Core::IEvent {
            uint32_t motionKey;
            float blendDuration;

            AnimationBlendStartedEvent(uint32_t key, float d) : motionKey(key), blendDuration(d) {}
        };

        struct AnimationStateChangedEvent : public UserInterface::Core::IEvent {
            UserInterface::InstanceControllers::MotionState oldState;
            UserInterface::InstanceControllers::MotionState newState;

            AnimationStateChangedEvent(UserInterface::InstanceControllers::MotionState o, UserInterface::InstanceControllers::MotionState n) 
                : oldState(o), newState(n) {}
        };
    }

    class InstanceAnimationController : public IInstanceAnimationController
    {
    public:
        InstanceAnimationController() = default;
        ~InstanceAnimationController() override = default;

        EterBase::PacketResult<void> PlayMotion(const MotionConfig& config) override
        {
            if (config.motionKey == 0)
            {
                EterBase::ModernLogger::Error("InstanceAnimationController: Attempted to play invalid motion");
                return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
            }

            EterBase::ModernLogger::Debug("InstanceAnimationController: Playing motion {} with speed {}", config.motionKey, config.speedMultiplier);
            
            m_currentMotionKey = config.motionKey;
            m_speed = config.speedMultiplier;
            m_isLooping = config.isLooping;
            
            ChangeState(DetermineState(config.motionKey));

            Core::EventBus::GetInstance().Publish(AnimationPlayedEvent{config.motionKey, config.speedMultiplier});
            return {};
        }

        EterBase::PacketResult<void> BlendMotion(uint32_t motionKey, float blendDuration) override
        {
            if (motionKey == 0)
            {
                EterBase::ModernLogger::Error("InstanceAnimationController: Attempted to blend invalid motion");
                return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
            }
            
            if (blendDuration <= 0.0f)
            {
                EterBase::ModernLogger::Warning("InstanceAnimationController: Invalid blend duration, delegating to PlayMotion");
                MotionConfig cfg;
                cfg.motionKey = motionKey;
                // Other fields left to their default values (e.g. blendTime{0.15f}, speedMultiplier{1.0f}, isLooping{false})
                return PlayMotion(cfg);
            }

            EterBase::ModernLogger::Debug("InstanceAnimationController: Blending motion {} over {}s", motionKey, blendDuration);
            
            m_currentMotionKey = motionKey;
            ChangeState(DetermineState(motionKey));

            Core::EventBus::GetInstance().Publish(AnimationBlendStartedEvent{motionKey, blendDuration});
            return {};
        }

        void SetMotionSpeed(float multiplier) override
        {
            if (multiplier < 0.0f)
            {
                EterBase::ModernLogger::Warning("InstanceAnimationController: Invalid speed multiplier {}", multiplier);
                return;
            }
            m_speed = multiplier;
        }

        bool IsMotionFinished() const override
        {
            return !m_isLooping; 
        }

        void CancelMotion() override
        {
            EterBase::ModernLogger::Debug("InstanceAnimationController: Motion cancelled");
            m_currentMotionKey = 0;
            m_isLooping = false;
            ChangeState(MotionState::Idle);
        }

        uint32_t GetCurrentMotion() const override
        {
            return m_currentMotionKey;
        }

        MotionState GetMotionState() const override
        {
            return m_currentState;
        }

        void Clear() override
        {
            EterBase::ModernLogger::Info("InstanceAnimationController: Clearing state");
            CancelMotion();
        }

    private:
        MotionState DetermineState(uint32_t motionKey)
        {
            // Using domain constants directly (assuming they are 1 to 6)
            switch (motionKey)
            {
                case 2: // CRaceMotionData::NAME_WALK 
                    return MotionState::Walk;
                case 3: // CRaceMotionData::NAME_RUN 
                    return MotionState::Run;
                case 13: // CRaceMotionData::NAME_NORMAL_ATTACK 
                    return MotionState::Attack;
                case 50: // CRaceMotionData::NAME_SKILL 
                    return MotionState::Skill;
                case 5: // CRaceMotionData::NAME_DAMAGE 
                    return MotionState::Damaged;
                case 11: // CRaceMotionData::NAME_DEAD 
                    return MotionState::Dead;
                default: return MotionState::Idle;
            }
        }

        void ChangeState(MotionState newState)
        {
            if (m_currentState != newState)
            {
                auto oldState = m_currentState;
                m_currentState = newState;
                Core::EventBus::GetInstance().Publish(AnimationStateChangedEvent{oldState, newState});
            }
        }

        uint32_t m_currentMotionKey{0};
        float m_speed{1.0f};
        bool m_isLooping{false};
        MotionState m_currentState{MotionState::Idle};
    };

    // Factory method for instantiation.
    std::unique_ptr<IInstanceAnimationController> CreateInstanceAnimationController()
    {
        return std::make_unique<InstanceAnimationController>();
    }
}
