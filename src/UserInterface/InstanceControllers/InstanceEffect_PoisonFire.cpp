#include "../StdAfx.h"
#include "IInstanceEffectController.h"
#include "src/EterBase/ModernLogger.h"
#include "src/UserInterface/Core/EventBus.h"

// Define arbitrary effect constants
namespace {
    constexpr uint8_t STATUS_POISON = 1 << 0;
    constexpr uint8_t STATUS_FIRE = 1 << 1;
    constexpr uint8_t STATUS_BLEEDING = 1 << 2;
    constexpr uint8_t STATUS_FREEZING = 1 << 3;

    struct StatusEffectVisualEvent : public UserInterface::Core::IEvent {
        uint8_t flag;
        bool active;
        StatusEffectVisualEvent(uint8_t f, bool a) : flag(f), active(a) {}
    };
}

namespace UserInterface::InstanceControllers
{
    class InstanceEffect_PoisonFire final : public IInstanceEffectController
    {
    private:
        bool m_isPoisoned{false};
        bool m_isAflame{false};
        bool m_isBleeding{false};
        bool m_isFreezing{false};

        void HandleEffectChange(uint8_t flag, bool& currentState, bool newState, std::string_view effectName)
        {
            if (currentState != newState) {
                currentState = newState;
                if (newState) {
                    EterBase::ModernLogger::Info("InstanceEffect_PoisonFire: {} effect started.", effectName);
                } else {
                    EterBase::ModernLogger::Info("InstanceEffect_PoisonFire: {} effect stopped.", effectName);
                }

                // Notify GUI or other systems via EventBus
                UserInterface::Core::EventBus::GetInstance().Publish(StatusEffectVisualEvent{flag, newState});
            }
        }

    public:
        InstanceEffect_PoisonFire() = default;
        ~InstanceEffect_PoisonFire() override = default;

        EterBase::PacketResult<uint32_t> AttachBoneEffect(const EffectAttachData& data) override
        {
            EterBase::ModernLogger::Debug("InstanceEffect_PoisonFire: AttachBoneEffect ID: {}", data.effectId);
            return 1; // Dummy effect handle
        }

        EterBase::PacketResult<void> DetachEffect(uint32_t handle) override
        {
            EterBase::ModernLogger::Debug("InstanceEffect_PoisonFire: DetachEffect handle: {}", handle);
            return {};
        }

        void SetSwordAura(bool active, uint32_t auraType) override
        {
            EterBase::ModernLogger::Debug("InstanceEffect_PoisonFire: SetSwordAura active: {}, type: {}", active, auraType);
        }

        void SetBuffVisual(uint32_t buffId, bool active) override
        {
            EterBase::ModernLogger::Debug("InstanceEffect_PoisonFire: SetBuffVisual ID: {}, active: {}", buffId, active);
        }

        void SetStatusEffect(uint8_t statusFlag, bool active) override
        {
            EterBase::ModernLogger::Debug("InstanceEffect_PoisonFire: SetStatusEffect flag: {}, active: {}", statusFlag, active);

            if (statusFlag & STATUS_POISON) {
                HandleEffectChange(STATUS_POISON, m_isPoisoned, active, "Poison");
            }
            if (statusFlag & STATUS_FIRE) {
                HandleEffectChange(STATUS_FIRE, m_isAflame, active, "Fire");
            }
            if (statusFlag & STATUS_BLEEDING) {
                HandleEffectChange(STATUS_BLEEDING, m_isBleeding, active, "Bleeding");
            }
            if (statusFlag & STATUS_FREEZING) {
                HandleEffectChange(STATUS_FREEZING, m_isFreezing, active, "Freezing");
            }
        }

        void SetItemShine(uint8_t partIndex, uint8_t refineLevel) override
        {
            EterBase::ModernLogger::Debug("InstanceEffect_PoisonFire: SetItemShine part: {}, refine: {}", partIndex, refineLevel);
        }

        void ClearAllEffects() override
        {
            EterBase::ModernLogger::Info("InstanceEffect_PoisonFire: Clearing all effects.");
            
            if (m_isPoisoned) HandleEffectChange(STATUS_POISON, m_isPoisoned, false, "Poison");
            if (m_isAflame) HandleEffectChange(STATUS_FIRE, m_isAflame, false, "Fire");
            if (m_isBleeding) HandleEffectChange(STATUS_BLEEDING, m_isBleeding, false, "Bleeding");
            if (m_isFreezing) HandleEffectChange(STATUS_FREEZING, m_isFreezing, false, "Freezing");
        }
    };
}

// Ensure the controller can be instantiated via an export factory if necessary
// This avoids it being dead code when someone wants to construct it outside the translation unit.
namespace UserInterface::InstanceControllers {
    std::unique_ptr<IInstanceEffectController> CreatePoisonFireEffectController() {
        return std::make_unique<InstanceEffect_PoisonFire>();
    }
}
