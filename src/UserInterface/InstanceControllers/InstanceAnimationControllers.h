#pragma once

#include <cstdint>
#include <string_view>
#include <memory>
#include <expected>
#include <queue>
#include <chrono>

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "../Core/EventBus.h"

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Stany animacji postaci/aktora.
     */
    enum class MotionState : uint8_t
    {
        Idle = 0,
        Walk,
        Run,
        Attack,
        Skill,
        Damaged,
        Dead
    };

    /**
     * @brief Konfiguracja odtwarzania ruchu animacji.
     */
    struct MotionConfig
    {
        uint32_t motionKey{0};
        float blendTime{0.15f};
        float speedMultiplier{1.0f};
        bool isLooping{false};
    };

    /**
     * @brief Powody przerwania biezacej animacji.
     */
    enum class CancelReason : uint8_t
    {
        Stun = 0,
        Knockback,
        Death
    };

    // --- Zdarzenia animacji ---

    struct AnimationPlayedEvent : public Core::IEvent
    {
        uint32_t motionKey{0};
        float speed{1.0f};

        AnimationPlayedEvent(uint32_t key, float s) : motionKey(key), speed(s) {}
    };

    struct AnimationBlendStartedEvent : public Core::IEvent
    {
        uint32_t motionKey{0};
        float blendDuration{0.0f};

        AnimationBlendStartedEvent(uint32_t key, float d) : motionKey(key), blendDuration(d) {}
    };

    struct AnimationStateChangedEvent : public Core::IEvent
    {
        MotionState oldState{MotionState::Idle};
        MotionState newState{MotionState::Idle};

        AnimationStateChangedEvent(MotionState o, MotionState n) : oldState(o), newState(n) {}
    };

    struct MotionCancelledEvent : public Core::IEvent
    {
        EterBase::EntityId entityId{0};
        uint32_t previousMotionKey{0};
        MotionState previousState{MotionState::Idle};
    };

    struct ComboStateChangedEvent : public Core::IEvent
    {
        uint32_t currentMotionKey{0};
        uint32_t queuedAttacks{0};
        bool isComboActive{false};

        ComboStateChangedEvent(uint32_t motionKey, uint32_t queuedAttacks, bool isActive)
            : currentMotionKey(motionKey), queuedAttacks(queuedAttacks), isComboActive(isActive) {}
    };

    struct AnimSpeedUpdatedEvent : public Core::IEvent
    {
        EterBase::EntityId entityId{0};
        float newSpeed{1.0f};
        MotionState currentState{MotionState::Idle};

        AnimSpeedUpdatedEvent(EterBase::EntityId id, float speed, MotionState state)
            : entityId(id), newSpeed(speed), currentState(state) {}
    };

    /**
     * @brief Glowny interfejs kontrolera animacji instancji.
     */
    class IInstanceAnimationController
    {
    public:
        virtual ~IInstanceAnimationController() = default;

        virtual EterBase::PacketResult<void> PlayMotion(const MotionConfig& config) = 0;
        virtual EterBase::PacketResult<void> BlendMotion(uint32_t motionKey, float blendDuration) = 0;
        virtual void SetMotionSpeed(float multiplier) = 0;
        virtual bool IsMotionFinished() const = 0;
        virtual void CancelMotion() = 0;
        virtual uint32_t GetCurrentMotion() const = 0;
        virtual MotionState GetMotionState() const = 0;
        virtual void Clear() = 0;
    };

    /**
     * @brief Podstawowy kontroler animacji obslugujacy odtwarzanie i blendowanie.
     */
    class InstanceAnimationController : public IInstanceAnimationController
    {
    public:
        InstanceAnimationController() = default;
        ~InstanceAnimationController() override = default;

        EterBase::PacketResult<void> PlayMotion(const MotionConfig& config) override;
        EterBase::PacketResult<void> BlendMotion(uint32_t motionKey, float blendDuration) override;
        void SetMotionSpeed(float multiplier) override;
        bool IsMotionFinished() const override;
        void CancelMotion() override;
        uint32_t GetCurrentMotion() const override;
        MotionState GetMotionState() const override;
        void Clear() override;

    private:
        MotionState DetermineState(uint32_t motionKey);
        void ChangeState(MotionState newState);

        uint32_t m_currentMotionKey{0};
        float m_speed{1.0f};
        bool m_isLooping{false};
        MotionState m_currentState{MotionState::Idle};
    };

    /**
     * @brief Kontroler animacji combo ze wsparciem kolejkowania atakow i okna czasowego.
     */
    class InstanceAnimComboController final : public IInstanceAnimationController
    {
    public:
        InstanceAnimComboController() = default;
        ~InstanceAnimComboController() override = default;

        EterBase::PacketResult<void> PlayMotion(const MotionConfig& config) override;
        EterBase::PacketResult<void> BlendMotion(uint32_t motionKey, float blendDuration) override;
        void SetMotionSpeed(float multiplier) override;
        bool IsMotionFinished() const override;
        void CancelMotion() override;
        uint32_t GetCurrentMotion() const override;
        MotionState GetMotionState() const override;
        void Clear() override;

        void Update();

    private:
        void ExecuteMotion(const MotionConfig& config);
        void ClearQueue();
        void PublishState();

        std::queue<MotionConfig> m_attackQueue;
        std::chrono::steady_clock::time_point m_lastAttackTime;
        MotionState m_state{MotionState::Idle};
        uint32_t m_currentMotionKey{0};
        float m_speedMultiplier{1.0f};

        static constexpr long long COMBO_WINDOW_MS = 1500;
        static constexpr size_t MAX_COMBO_STEPS = 4;
        static constexpr float BASE_ANIMATION_DURATION_MS = 1000.0f;
    };

    /**
     * @brief Implementacja kontrolera animacji z bezposrednia emisja zdarzen klatek trafienia.
     */
    class InstanceAnimationControllerEventsImpl final : public IInstanceAnimationController
    {
    public:
        explicit InstanceAnimationControllerEventsImpl(EterBase::EntityId entityId);
        ~InstanceAnimationControllerEventsImpl() override = default;

        void OnHitFrame(uint8_t hitIndex);
        void OnAnimationFinished();

        EterBase::PacketResult<void> PlayMotion(const MotionConfig& config) override;
        EterBase::PacketResult<void> BlendMotion(uint32_t motionKey, float blendDuration) override;
        void SetMotionSpeed(float multiplier) override;
        bool IsMotionFinished() const override;
        void CancelMotion() override;
        uint32_t GetCurrentMotion() const override;
        MotionState GetMotionState() const override;
        void Clear() override;

    private:
        EterBase::EntityId m_entityId{0};
        uint32_t m_currentMotionKey{0};
        MotionState m_state{MotionState::Idle};
        float m_speed{1.0f};
    };

    /**
     * @brief Handler anulowania animacji na skutek ogluszenia, odrzutu badz zgonu.
     */
    class InstanceAnim_Cancel
    {
    public:
        InstanceAnim_Cancel(EterBase::EntityId entityId, IInstanceAnimationController* controller);

        EterBase::PacketResult<void> CancelMotionOnEvent(CancelReason reason);

    private:
        EterBase::EntityId m_entityId{0};
        IInstanceAnimationController* m_controller{nullptr};
    };

    /**
     * @brief Modul zarzadzajacy predkoscia animacji (ruch, atak).
     */
    class InstanceAnimSpeedHandler
    {
    public:
        static std::expected<void, EterBase::EntityError> UpdateAnimationSpeed(
            IInstanceAnimationController& controller,
            EterBase::EntityId entityId,
            float attackSpeedMultiplier,
            float movementSpeedMultiplier);
    };

    // --- Metody fabryczne ---
    std::unique_ptr<IInstanceAnimationController> CreateInstanceAnimationController();
    std::unique_ptr<IInstanceAnimationController> CreateInstanceAnimationController(EterBase::EntityId entityId);
    std::unique_ptr<IInstanceAnimationController> CreateInstanceAnimComboController();
}
