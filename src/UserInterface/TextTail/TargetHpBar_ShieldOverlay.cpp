#include "../StdAfx.h"
#include "ITargetHpBarService.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "UserInterface/Core/EventBus.h"
#include <memory>

namespace UserInterface::TextTail
{
    class TargetHpBarShieldOverlay : public ITargetHpBarService
    {
    public:
        TargetHpBarShieldOverlay() = default;
        ~TargetHpBarShieldOverlay() override = default;

        void ShowTargetBar(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) override
        {
            EterBase::ModernLogger::Info("TargetHpBarShieldOverlay: Showing target bar for VID: {}", vid.value());
            targetVid_ = vid;
            currentHp_ = currentHp;
            maxHp_ = maxHp;
            shieldAmount_ = 0; // Reset shield on show
            isVisible_ = true;
        }

        void UpdateTargetHp(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) override
        {
            if (vid != targetVid_)
            {
                EterBase::ModernLogger::Warning("TargetHpBarShieldOverlay: UpdateTargetHp for mismatched VID. Expected: {}, Got: {}", targetVid_.value(), vid.value());
                return;
            }

            EterBase::ModernLogger::Debug("TargetHpBarShieldOverlay: Updating HP for VID: {} - HP: {}/{}", vid.value(), currentHp, maxHp);
            currentHp_ = currentHp;
            maxHp_ = maxHp;

            Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent(vid.value()));
        }

        // New method specific to the shield overlay functionality
        void UpdateTargetShield(EterBase::EntityId vid, uint32_t shieldAmount)
        {
            if (vid != targetVid_)
            {
                EterBase::ModernLogger::Warning("TargetHpBarShieldOverlay: UpdateTargetShield for mismatched VID. Expected: {}, Got: {}", targetVid_.value(), vid.value());
                return;
            }

            EterBase::ModernLogger::Debug("TargetHpBarShieldOverlay: Updating Shield for VID: {} - Shield: {}", vid.value(), shieldAmount);
            shieldAmount_ = shieldAmount;
            
            Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent(vid.value()));
        }

        void HideTargetBar() override
        {
            EterBase::ModernLogger::Info("TargetHpBarShieldOverlay: Hiding target bar for VID: {}", targetVid_.value());
            isVisible_ = false;
        }

        void RenderTargetBar() override
        {
            if (!isVisible_)
            {
                return;
            }

            EterBase::ModernLogger::Trace("TargetHpBarShieldOverlay: Rendering target HP bar for VID: {}", targetVid_.value());
            
            // Logic to render the blue absorption shield overlay
            if (shieldAmount_ > 0)
            {
                EterBase::ModernLogger::Trace("TargetHpBarShieldOverlay: Rendering blue absorption shield overlay on top of HP bar. Shield Amount: {}", shieldAmount_);
                // Real rendering implementation (e.g., drawing blue quads) would go here
            }
        }

        void RenderMiniMobBars() override
        {
            EterBase::ModernLogger::Trace("TargetHpBarShieldOverlay: Rendering mini mob bars");
            // Mini mob bars shield logic could also be added here if required
        }

        void Clear() override
        {
            EterBase::ModernLogger::Info("TargetHpBarShieldOverlay: Clearing state");
            targetVid_ = EterBase::EntityId{0};
            currentHp_ = 0;
            maxHp_ = 0;
            shieldAmount_ = 0;
            isVisible_ = false;
        }

    private:
        EterBase::EntityId targetVid_{0};
        uint32_t currentHp_{0};
        uint32_t maxHp_{0};
        uint32_t shieldAmount_{0}; // State for the absorption shield
        bool isVisible_{false};
    };

    // Factory function to expose the class to the rest of the application
    std::unique_ptr<ITargetHpBarService> CreateTargetHpBarShieldOverlay()
    {
        return std::make_unique<TargetHpBarShieldOverlay>();
    }
}
