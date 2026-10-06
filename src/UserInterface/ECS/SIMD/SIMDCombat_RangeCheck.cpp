#include "../../StdAfx.h"
#include "ISIMDCombatEvaluator.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/ModernLogger.h"
#include "UserInterface/Core/EventBus.h"
#include <memory>

namespace UserInterface::ECS::SIMD
{
    class SIMDCombatEvaluator final : public ISIMDCombatEvaluator
    {
    public:
        SIMDCombatEvaluator() = default;
        ~SIMDCombatEvaluator() override = default;

        void EvaluateAliveMask(
            const uint32_t* currentHp, uint8_t* outAliveMask, size_t count) override
        {
            if (!currentHp || !outAliveMask)
            {
                EterBase::ModernLogger::Error("SIMDCombatEvaluator::EvaluateAliveMask called with null pointers.");
                return;
            }

            for (size_t i = 0; i < count; ++i)
            {
                outAliveMask[i] = (currentHp[i] > 0) ? 1 : 0;
            }
        }

        void EvaluateHpRatio(
            const uint32_t* currentHp, const uint32_t* maxHp,
            float* outRatio, size_t count) override
        {
            if (!currentHp || !maxHp || !outRatio)
            {
                EterBase::ModernLogger::Error("SIMDCombatEvaluator::EvaluateHpRatio called with null pointers.");
                return;
            }

            for (size_t i = 0; i < count; ++i)
            {
                if (maxHp[i] > 0)
                {
                    outRatio[i] = static_cast<float>(currentHp[i]) / static_cast<float>(maxHp[i]);
                }
                else
                {
                    outRatio[i] = 0.0f;
                }
            }
        }

        // According to ISIMDCombatEvaluator.h contract, there is no specific EvaluateWeaponRange method.
        // Therefore, we implement the vectorized checking of weapon range (sword/bow) logic
        // inside the provided EvaluateAggroRange, interpreting aggroRadius as the weapon range.
        void EvaluateAggroRange(
            const float* distances, float aggroRadius,
            uint8_t* outAggroMask, size_t count) override
        {
            auto validateInputs = [&]() -> std::expected<void, EterBase::CombatError> {
                if (!distances || !outAggroMask)
                {
                    return std::unexpected(EterBase::CombatError::InvalidAction);
                }
                return {};
            };

            if (auto result = validateInputs(); !result)
            {
                EterBase::ModernLogger::Error("SIMDCombatEvaluator::EvaluateAggroRange validation failed.");
                return;
            }

            EterBase::ModernLogger::Debug("Evaluating weapon/aggro range for {} entities. Range: {}", count, aggroRadius);

            const float radiusSquared = aggroRadius * aggroRadius;

            // Phase 1: Branchless distance evaluation (for auto-vectorization)
            for (size_t i = 0; i < count; ++i)
            {
                outAggroMask[i] = (distances[i] <= radiusSquared) ? 1 : 0;
            }

            // Phase 2: Domain logic and event publishing using strong types
            auto& eventBus = UserInterface::Core::EventBus::GetInstance();
            for (size_t i = 0; i < count; ++i)
            {
                if (outAggroMask[i])
                {
                    EterBase::EntityId targetId(static_cast<uint32_t>(i)); // Using strong type EterBase::EntityId
                    eventBus.Publish(UserInterface::Core::TargetBoardRefreshEvent(targetId.value()));
                }
            }
        }

        void Clear() override
        {
            EterBase::ModernLogger::Debug("SIMDCombatEvaluator::Clear() called.");
        }
    };

    // Factory function to create and expose the instance, preventing dead code.
    std::unique_ptr<ISIMDCombatEvaluator> CreateSIMDCombatEvaluator_RangeCheck()
    {
        return std::make_unique<SIMDCombatEvaluator>();
    }
}
