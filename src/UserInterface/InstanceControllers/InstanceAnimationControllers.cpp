#include "../StdAfx.h"
#include "InstanceAnimationControllers.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::InstanceControllers
{
    // =========================================================================
    // InstanceAnimationController
    // =========================================================================

    EterBase::PacketResult<void> InstanceAnimationController::PlayMotion(const MotionConfig& config)
    {
        if (config.motionKey == 0)
        {
            EterBase::ModernLogger::Error("InstanceAnimationController: Attempted to play invalid motion");
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }

        EterBase::ModernLogger::Debug("InstanceAnimationController: Playing motion {} with speed {}",
            config.motionKey, config.speedMultiplier);

        m_currentMotionKey = config.motionKey;
        m_speed = config.speedMultiplier;
        m_isLooping = config.isLooping;

        ChangeState(DetermineState(config.motionKey));

        Core::EventBus::GetInstance().Publish(AnimationPlayedEvent{config.motionKey, config.speedMultiplier});
        return {};
    }

    EterBase::PacketResult<void> InstanceAnimationController::BlendMotion(uint32_t motionKey, float blendDuration)
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
            return PlayMotion(cfg);
        }

        EterBase::ModernLogger::Debug("InstanceAnimationController: Blending motion {} over {}s", motionKey, blendDuration);

        m_currentMotionKey = motionKey;
        ChangeState(DetermineState(motionKey));

        Core::EventBus::GetInstance().Publish(AnimationBlendStartedEvent{motionKey, blendDuration});
        return {};
    }

    void InstanceAnimationController::SetMotionSpeed(float multiplier)
    {
        if (multiplier < 0.0f)
        {
            EterBase::ModernLogger::Warning("InstanceAnimationController: Invalid speed multiplier {}", multiplier);
            return;
        }
        m_speed = multiplier;
    }

    bool InstanceAnimationController::IsMotionFinished() const
    {
        return !m_isLooping;
    }

    void InstanceAnimationController::CancelMotion()
    {
        EterBase::ModernLogger::Debug("InstanceAnimationController: Motion cancelled");
        m_currentMotionKey = 0;
        m_isLooping = false;
        ChangeState(MotionState::Idle);
    }

    uint32_t InstanceAnimationController::GetCurrentMotion() const
    {
        return m_currentMotionKey;
    }

    MotionState InstanceAnimationController::GetMotionState() const
    {
        return m_currentState;
    }

    void InstanceAnimationController::Clear()
    {
        EterBase::ModernLogger::Info("InstanceAnimationController: Clearing state");
        CancelMotion();
    }

    MotionState InstanceAnimationController::DetermineState(uint32_t motionKey)
    {
        switch (motionKey)
        {
            case 2:  // CRaceMotionData::NAME_WALK
                return MotionState::Walk;
            case 3:  // CRaceMotionData::NAME_RUN
                return MotionState::Run;
            case 13: // CRaceMotionData::NAME_NORMAL_ATTACK
                return MotionState::Attack;
            case 50: // CRaceMotionData::NAME_SKILL
                return MotionState::Skill;
            case 5:  // CRaceMotionData::NAME_DAMAGE
                return MotionState::Damaged;
            case 11: // CRaceMotionData::NAME_DEAD
                return MotionState::Dead;
            default:
                return MotionState::Idle;
        }
    }

    void InstanceAnimationController::ChangeState(MotionState newState)
    {
        if (m_currentState != newState)
        {
            auto oldState = m_currentState;
            m_currentState = newState;
            Core::EventBus::GetInstance().Publish(AnimationStateChangedEvent{oldState, newState});
        }
    }

    // =========================================================================
    // InstanceAnimComboController
    // =========================================================================

    EterBase::PacketResult<void> InstanceAnimComboController::PlayMotion(const MotionConfig& config)
    {
        auto now = std::chrono::steady_clock::now();

        if (m_state == MotionState::Attack)
        {
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastAttackTime).count();
            if (elapsedMs <= COMBO_WINDOW_MS)
            {
                if (m_attackQueue.size() < MAX_COMBO_STEPS)
                {
                    m_attackQueue.push(config);
                    EterBase::ModernLogger::Debug("Combo attack queued. Pending attacks: {}", m_attackQueue.size());
                    PublishState();
                    return {};
                }
                else
                {
                    EterBase::ModernLogger::Info("Combo sequence reached maximum limit.");
                    return {};
                }
            }
            else
            {
                EterBase::ModernLogger::Info("Combo time window expired. Resetting combo sequence.");
                ClearQueue();
            }
        }

        ExecuteMotion(config);
        return {};
    }

    EterBase::PacketResult<void> InstanceAnimComboController::BlendMotion(uint32_t motionKey, float blendDuration)
    {
        EterBase::ModernLogger::Debug("Blending motion to: {} over {}s", motionKey, blendDuration);
        m_currentMotionKey = motionKey;
        m_state = MotionState::Skill;
        PublishState();
        return {};
    }

    void InstanceAnimComboController::SetMotionSpeed(float multiplier)
    {
        m_speedMultiplier = multiplier;
    }

    bool InstanceAnimComboController::IsMotionFinished() const
    {
        if (m_state == MotionState::Idle)
        {
            return true;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastAttackTime).count();

        float actualDuration = BASE_ANIMATION_DURATION_MS / (m_speedMultiplier > 0.0f ? m_speedMultiplier : 1.0f);
        return elapsedMs > static_cast<long long>(actualDuration);
    }

    void InstanceAnimComboController::CancelMotion()
    {
        EterBase::ModernLogger::Info("Motion cancelled.");
        Clear();
    }

    uint32_t InstanceAnimComboController::GetCurrentMotion() const
    {
        return m_currentMotionKey;
    }

    MotionState InstanceAnimComboController::GetMotionState() const
    {
        return m_state;
    }

    void InstanceAnimComboController::Clear()
    {
        ClearQueue();
        m_state = MotionState::Idle;
        m_currentMotionKey = 0;
        PublishState();
    }

    void InstanceAnimComboController::Update()
    {
        if (m_state == MotionState::Attack && IsMotionFinished())
        {
            if (!m_attackQueue.empty())
            {
                auto nextConfig = m_attackQueue.front();
                m_attackQueue.pop();
                EterBase::ModernLogger::Debug("Executing next queued combo attack.");
                ExecuteMotion(nextConfig);
            }
            else
            {
                m_state = MotionState::Idle;
                PublishState();
            }
        }
    }

    void InstanceAnimComboController::ExecuteMotion(const MotionConfig& config)
    {
        m_currentMotionKey = config.motionKey;
        m_speedMultiplier = config.speedMultiplier;
        m_lastAttackTime = std::chrono::steady_clock::now();
        m_state = MotionState::Attack;

        EterBase::ModernLogger::Info("Executing attack motion: {}", m_currentMotionKey);
        PublishState();
    }

    void InstanceAnimComboController::ClearQueue()
    {
        std::queue<MotionConfig> emptyQueue;
        std::swap(m_attackQueue, emptyQueue);
    }

    void InstanceAnimComboController::PublishState()
    {
        Core::EventBus::GetInstance().Publish(ComboStateChangedEvent(
            m_currentMotionKey,
            static_cast<uint32_t>(m_attackQueue.size()),
            m_state == MotionState::Attack
        ));
    }

    // =========================================================================
    // InstanceAnimationControllerEventsImpl
    // =========================================================================

    InstanceAnimationControllerEventsImpl::InstanceAnimationControllerEventsImpl(EterBase::EntityId entityId)
        : m_entityId(entityId)
    {
    }

    void InstanceAnimationControllerEventsImpl::OnHitFrame(uint8_t hitIndex)
    {
        Core::EventBus::GetInstance().Publish(Core::AnimHitFrameEvent(m_entityId, m_currentMotionKey, hitIndex));
        EterBase::ModernLogger::Debug("Entity {} emitted AnimHitFrameEvent (Key: {}, HitIndex: {})",
            m_entityId.value(), m_currentMotionKey, hitIndex);
    }

    void InstanceAnimationControllerEventsImpl::OnAnimationFinished()
    {
        Core::EventBus::GetInstance().Publish(Core::AnimFinishedEvent(m_entityId, m_currentMotionKey));
        EterBase::ModernLogger::Debug("Entity {} emitted AnimFinishedEvent (Key: {})",
            m_entityId.value(), m_currentMotionKey);
    }

    EterBase::PacketResult<void> InstanceAnimationControllerEventsImpl::PlayMotion(const MotionConfig& config)
    {
        m_currentMotionKey = config.motionKey;
        m_state = MotionState::Skill;
        EterBase::ModernLogger::Trace("Entity {} playing motion {}", m_entityId.value(), m_currentMotionKey);
        return {};
    }

    EterBase::PacketResult<void> InstanceAnimationControllerEventsImpl::BlendMotion(uint32_t motionKey, float /*blendDuration*/)
    {
        m_currentMotionKey = motionKey;
        return {};
    }

    void InstanceAnimationControllerEventsImpl::SetMotionSpeed(float multiplier)
    {
        m_speed = multiplier;
    }

    bool InstanceAnimationControllerEventsImpl::IsMotionFinished() const
    {
        return m_state == MotionState::Idle;
    }

    void InstanceAnimationControllerEventsImpl::CancelMotion()
    {
        m_currentMotionKey = 0;
        m_state = MotionState::Idle;
    }

    uint32_t InstanceAnimationControllerEventsImpl::GetCurrentMotion() const
    {
        return m_currentMotionKey;
    }

    MotionState InstanceAnimationControllerEventsImpl::GetMotionState() const
    {
        return m_state;
    }

    void InstanceAnimationControllerEventsImpl::Clear()
    {
        m_currentMotionKey = 0;
        m_state = MotionState::Idle;
        m_speed = 1.0f;
    }

    // =========================================================================
    // InstanceAnim_Cancel
    // =========================================================================

    InstanceAnim_Cancel::InstanceAnim_Cancel(EterBase::EntityId entityId, IInstanceAnimationController* controller)
        : m_entityId(entityId), m_controller(controller)
    {
    }

    EterBase::PacketResult<void> InstanceAnim_Cancel::CancelMotionOnEvent(CancelReason reason)
    {
        if (!m_controller)
        {
            EterBase::ModernLogger::Error("InstanceAnim_Cancel::CancelMotionOnEvent - Null animation controller provided");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        EterBase::ModernLogger::Info("InstanceAnim_Cancel::CancelMotionOnEvent - Cancelling motion for EntityId: {}, Reason: {}",
            m_entityId.value(), static_cast<uint8_t>(reason));

        MotionCancelledEvent event;
        event.entityId = m_entityId;
        event.previousMotionKey = m_controller->GetCurrentMotion();
        event.previousState = m_controller->GetMotionState();

        m_controller->CancelMotion();
        Core::EventBus::GetInstance().Publish(event);

        return {};
    }

    // =========================================================================
    // InstanceAnimSpeedHandler
    // =========================================================================

    std::expected<void, EterBase::EntityError> InstanceAnimSpeedHandler::UpdateAnimationSpeed(
        IInstanceAnimationController& controller,
        EterBase::EntityId entityId,
        float attackSpeedMultiplier,
        float movementSpeedMultiplier)
    {
        if (!entityId)
        {
            EterBase::ModernLogger::Error("InstanceAnimSpeedHandler: Otrzymano nieprawidlowy EntityId.");
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        const MotionState state = controller.GetMotionState();
        float targetSpeed = 1.0f;

        switch (state)
        {
            case MotionState::Walk:
            case MotionState::Run:
                targetSpeed = movementSpeedMultiplier;
                break;
            case MotionState::Attack:
            case MotionState::Skill:
                targetSpeed = attackSpeedMultiplier;
                break;
            case MotionState::Idle:
            case MotionState::Damaged:
            case MotionState::Dead:
                targetSpeed = 1.0f;
                break;
            default:
                EterBase::ModernLogger::Warning("InstanceAnimSpeedHandler: Nieznany stan animacji.");
                targetSpeed = 1.0f;
                break;
        }

        controller.SetMotionSpeed(targetSpeed);

        EterBase::ModernLogger::Debug("InstanceAnimSpeedHandler: Ustawiono predkosc {} dla EntityId {} w stanie {}",
            targetSpeed, static_cast<uint32_t>(entityId.value()), static_cast<uint8_t>(state));

        UserInterface::Core::EventBus::GetInstance().Publish(AnimSpeedUpdatedEvent(entityId, targetSpeed, state));

        return {};
    }

    // =========================================================================
    // Fabryki
    // =========================================================================

    std::unique_ptr<IInstanceAnimationController> CreateInstanceAnimationController()
    {
        return std::make_unique<InstanceAnimationController>();
    }

    std::unique_ptr<IInstanceAnimationController> CreateInstanceAnimationController(EterBase::EntityId entityId)
    {
        return std::make_unique<InstanceAnimationControllerEventsImpl>(entityId);
    }

    std::unique_ptr<IInstanceAnimationController> CreateInstanceAnimComboController()
    {
        return std::make_unique<InstanceAnimComboController>();
    }
}
