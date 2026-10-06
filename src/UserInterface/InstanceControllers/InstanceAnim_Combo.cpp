#include "../StdAfx.h"
#include "IInstanceAnimationController.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/ModernLogger.h"
#include "../Core/EventBus.h"

#include <queue>
#include <chrono>
#include <optional>

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Event published when the combo state changes, updating other systems or the UI.
     */
    struct ComboStateChangedEvent : public Core::IEvent
    {
        uint32_t currentMotionKey;
        uint32_t queuedAttacks;
        bool isComboActive;

        ComboStateChangedEvent(uint32_t motionKey, uint32_t queuedAttacks, bool isActive)
            : currentMotionKey(motionKey), queuedAttacks(queuedAttacks), isComboActive(isActive) {}
    };

    /**
     * @brief Implementation of IInstanceAnimationController for handling combo attacks.
     * 
     * Manages a time window for accepting follow-up attacks and queuing them for seamless
     * animation blending. Strictly follows the Zero-Conflict rule by residing entirely in the .cpp file.
     */
    class InstanceAnimComboController final : public IInstanceAnimationController
    {
    public:
        InstanceAnimComboController() = default;
        ~InstanceAnimComboController() override = default;

        EterBase::PacketResult<void> PlayMotion(const MotionConfig& config) override
        {
            auto now = std::chrono::steady_clock::now();

            if (state == MotionState::Attack)
            {
                auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAttackTime).count();
                if (elapsedMs <= comboWindowMs)
                {
                    if (attackQueue.size() < maxComboSteps)
                    {
                        attackQueue.push(config);
                        EterBase::ModernLogger::Debug("Combo attack queued. Pending attacks: {}", attackQueue.size());
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

        EterBase::PacketResult<void> BlendMotion(uint32_t motionKey, float blendDuration) override
        {
            EterBase::ModernLogger::Debug("Blending motion to: {} over {}s", motionKey, blendDuration);
            currentMotionKey = motionKey;
            state = MotionState::Skill; 
            PublishState();
            return {};
        }

        void SetMotionSpeed(float multiplier) override
        {
            speedMultiplier = multiplier;
        }

        bool IsMotionFinished() const override
        {
            if (state == MotionState::Idle)
            {
                return true;
            }
            
            auto now = std::chrono::steady_clock::now();
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAttackTime).count();
            
            float actualDuration = baseAnimationDurationMs / (speedMultiplier > 0.0f ? speedMultiplier : 1.0f);
            return elapsedMs > static_cast<long long>(actualDuration);
        }

        void CancelMotion() override
        {
            EterBase::ModernLogger::Info("Motion cancelled.");
            Clear();
        }

        uint32_t GetCurrentMotion() const override
        {
            return currentMotionKey;
        }

        MotionState GetMotionState() const override
        {
            return state;
        }

        void Clear() override
        {
            ClearQueue();
            state = MotionState::Idle;
            currentMotionKey = 0;
            PublishState();
        }

        /**
         * @brief Called cyclically to check and advance the combo queue if current motion is finished.
         */
        void Update()
        {
            if (state == MotionState::Attack && IsMotionFinished())
            {
                if (!attackQueue.empty())
                {
                    auto nextConfig = attackQueue.front();
                    attackQueue.pop();
                    EterBase::ModernLogger::Debug("Executing next queued combo attack.");
                    ExecuteMotion(nextConfig);
                }
                else
                {
                    state = MotionState::Idle;
                    PublishState();
                }
            }
        }

    private:
        void ExecuteMotion(const MotionConfig& config)
        {
            currentMotionKey = config.motionKey;
            speedMultiplier = config.speedMultiplier;
            lastAttackTime = std::chrono::steady_clock::now();
            state = MotionState::Attack;
            
            EterBase::ModernLogger::Info("Executing attack motion: {}", currentMotionKey);
            PublishState();
        }

        void ClearQueue()
        {
            std::queue<MotionConfig> emptyQueue;
            std::swap(attackQueue, emptyQueue);
        }

        void PublishState()
        {
            Core::EventBus::GetInstance().Publish(ComboStateChangedEvent(
                currentMotionKey,
                static_cast<uint32_t>(attackQueue.size()),
                state == MotionState::Attack
            ));
        }

        std::queue<MotionConfig> attackQueue;
        std::chrono::steady_clock::time_point lastAttackTime;
        
        MotionState state{MotionState::Idle};
        uint32_t currentMotionKey{0};
        float speedMultiplier{1.0f};
        
        // Configurable constraints for combo window
        static constexpr long long comboWindowMs = 1500;
        static constexpr size_t maxComboSteps = 4;
        static constexpr float baseAnimationDurationMs = 1000.0f;
    };


    /**
     * @brief Factory function to instantiate the combo controller.
     */
    std::unique_ptr<IInstanceAnimationController> CreateInstanceAnimComboController()
    {
        return std::make_unique<InstanceAnimComboController>();
    }

}
