#include "../StdAfx.h"
#include "ITargetHpBarService.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include <format>
#include <optional>
#include <memory>

namespace UserInterface::TextTail
{
    struct TargetHpBarShowEvent : public Core::IEvent
    {
        EterBase::EntityId targetVid;
        uint32_t currentHp{ 0 };
        uint32_t maxHp{ 0 };

        TargetHpBarShowEvent(EterBase::EntityId vid, uint32_t cur, uint32_t max)
            : targetVid(vid), currentHp(cur), maxHp(max) {}
    };

    struct TargetHpBarHideEvent : public Core::IEvent
    {
    };

    namespace
    {
        struct TargetHpBarState
        {
            EterBase::EntityId targetVid;
            uint32_t currentHp{ 0 };
            uint32_t maxHp{ 0 };
            bool isVisible{ false };
        };
    }

    class TargetHpBarService final : public ITargetHpBarService
    {
    public:
        TargetHpBarService() = default;
        ~TargetHpBarService() override = default;

        void ShowTargetBar(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) override
        {
            if (vid.value() == 0)
            {
                EterBase::ModernLogger::Error("TargetHpBarService: Attempted to show HP bar for invalid entity ID");
                return;
            }

            m_state.emplace(TargetHpBarState{ vid, currentHp, maxHp, true });

            EterBase::ModernLogger::Info("TargetHpBarService: Showing HP bar for VID {}, HP: {}/{}", vid.value(), currentHp, maxHp);

            Core::EventBus::GetInstance().Publish(TargetHpBarShowEvent{ vid, currentHp, maxHp });
            Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent{ vid.value() });
        }

        void UpdateTargetHp(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) override
        {
            if (!m_state.has_value() || m_state->targetVid != vid)
            {
                EterBase::ModernLogger::Debug("TargetHpBarService: Ignoring HP update for inactive target VID {}", vid.value());
                return;
            }

            m_state->currentHp = currentHp;
            m_state->maxHp = maxHp;

            EterBase::ModernLogger::Debug("TargetHpBarService: Updated target HP for VID {}, HP: {}/{}", vid.value(), currentHp, maxHp);

            Core::EventBus::GetInstance().Publish(TargetHpBarShowEvent{ vid, currentHp, maxHp });
            Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent{ vid.value() });
        }

        void HideTargetBar() override
        {
            if (!m_state.has_value() || !m_state->isVisible)
                return;

            EterBase::ModernLogger::Info("TargetHpBarService: Hiding HP bar for VID {}", m_state->targetVid.value());
            m_state->isVisible = false;
            
            Core::EventBus::GetInstance().Publish(TargetHpBarHideEvent{});
        }

        void RenderTargetBar() override {}
        void RenderMiniMobBars() override {}

        void Clear() override
        {
            EterBase::ModernLogger::Info("TargetHpBarService: Clearing HP bar state");
            HideTargetBar();
            m_state.reset();
        }

    private:
        std::optional<TargetHpBarState> m_state;
    };

    std::unique_ptr<ITargetHpBarService> CreateTargetHpBarService_Show()
    {
        return std::make_unique<TargetHpBarService>();
    }
}
