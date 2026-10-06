#include "../StdAfx.h"
#include "ITargetHpBarService.h"
#include "../Core/EventBus.h"
#include "../Core/Events.h"
#include "EterBase/ModernLogger.h"

namespace UserInterface::TextTail
{
    /**
     * @brief Event-based implementation of ITargetHpBarService.
     * 
     * Handles updating target HP by publishing events to the EventBus instead of 
     * direct UI calls, achieving decoupling.
     */
    class TargetHpBarService_Events final : public ITargetHpBarService
    {
    public:
        TargetHpBarService_Events() = default;
        ~TargetHpBarService_Events() override = default;

        /**
         * @brief Shows the target bar for the specified entity.
         * @param vid The unique identifier of the target entity.
         * @param currentHp The entity's current HP.
         * @param maxHp The entity's maximum HP.
         */
        void ShowTargetBar(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) override
        {
            EterBase::ModernLogger::Debug("TargetHpBarService_Events::ShowTargetBar: VID {}, HP {}/{}", vid.value(), currentHp, maxHp);
            m_currentTarget = vid;
            
            ::Core::Events::HpChange event{
                .targetId = vid.value(),
                .currentHp = currentHp,
                .maxHp = maxHp
            };
            ::UserInterface::Core::EventBus::GetInstance().Publish(event);
        }

        /**
         * @brief Updates the target bar for the specified entity.
         * @param vid The unique identifier of the target entity.
         * @param currentHp The entity's current HP.
         * @param maxHp The entity's maximum HP.
         */
        void UpdateTargetHp(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) override
        {
            EterBase::ModernLogger::Debug("TargetHpBarService_Events::UpdateTargetHp: VID {}, HP {}/{}", vid.value(), currentHp, maxHp);
            m_currentTarget = vid;

            ::Core::Events::HpChange event{
                .targetId = vid.value(),
                .currentHp = currentHp,
                .maxHp = maxHp
            };
            ::UserInterface::Core::EventBus::GetInstance().Publish(event);
        }

        /**
         * @brief Hides the current target bar.
         */
        void HideTargetBar() override
        {
            EterBase::ModernLogger::Debug("TargetHpBarService_Events::HideTargetBar called");
            if (m_currentTarget)
            {
                ::Core::Events::TargetDelete event{
                    .targetId = m_currentTarget.value()
                };
                ::UserInterface::Core::EventBus::GetInstance().Publish(event);
                m_currentTarget = EterBase::EntityId{};
            }
        }

        /**
         * @brief Renders the target bar.
         */
        void RenderTargetBar() override
        {
            // Handled by UI events/layer, nothing to do here.
        }

        /**
         * @brief Renders mini mob bars.
         */
        void RenderMiniMobBars() override
        {
            // Handled by UI events/layer, nothing to do here.
        }

        /**
         * @brief Clears the target bar state.
         */
        void Clear() override
        {
            EterBase::ModernLogger::Debug("TargetHpBarService_Events::Clear called");
            HideTargetBar();
        }

    private:
        EterBase::EntityId m_currentTarget{};
    };
}
