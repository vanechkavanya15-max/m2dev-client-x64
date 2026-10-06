#include "../StdAfx.h"
#include "ITargetHpBarService.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../PythonTextTail.h"
#include "../../EterPythonLib/PythonGraphic.h"
#include <optional>

namespace UserInterface::TextTail
{
    class TargetHpBarService final : public ITargetHpBarService
    {
    public:
        TargetHpBarService() = default;
        ~TargetHpBarService() override = default;

        void ShowTargetBar(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) override
        {
            EterBase::ModernLogger::Info("Showing target HP bar for entity {}, HP: {}/{}", vid.value(), currentHp, maxHp);
            m_targetVid = vid;
            m_currentHp = currentHp;
            m_maxHp = maxHp;
            
            Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent(vid.value()));
        }

        void UpdateTargetHp(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) override
        {
            if (m_targetVid && m_targetVid->value() == vid.value())
            {
                EterBase::ModernLogger::Debug("Updating target HP bar for entity {}, HP: {}/{}", vid.value(), currentHp, maxHp);
                m_currentHp = currentHp;
                m_maxHp = maxHp;
                
                Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent(vid.value()));
            }
        }

        void HideTargetBar() override
        {
            if (m_targetVid)
            {
                EterBase::ModernLogger::Info("Hiding target HP bar for entity {}", m_targetVid->value());
                m_targetVid = std::nullopt;
            }
        }

        void RenderTargetBar() override
        {
            if (!m_targetVid)
                return;
                
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            if (CPythonTextTail::Instance().GetTextTailPosition(m_targetVid->value(), &x, &y, &z)) {
                // Determine target text size offset (since we only have the top point 'y' exposed in API)
                // We add an offset to align it right below the name text. A typical Metin2 text height is ~15 units.
                float yOffset = 15.0f;
                float hpBarY = y + yOffset;
                
                float barWidth = 100.0f;
                float barHeight = 7.0f;
                float sx = x - (barWidth / 2.0f);
                float ex = sx + barWidth;
                
                // Draw background
                CPythonGraphic::Instance().SetDiffuseColor(0.1f, 0.1f, 0.1f, 0.8f);
                CPythonGraphic::Instance().RenderBar2d(sx, hpBarY, ex, hpBarY + barHeight);
                
                // Draw HP
                if (m_maxHp > 0)
                {
                    float hpPercent = static_cast<float>(m_currentHp) / static_cast<float>(m_maxHp);
                    if (hpPercent > 1.0f) hpPercent = 1.0f;
                    
                    float currentEx = sx + (barWidth * hpPercent);
                    CPythonGraphic::Instance().SetDiffuseColor(1.0f, 0.1f, 0.1f, 0.9f);
                    CPythonGraphic::Instance().RenderBar2d(sx, hpBarY, currentEx, hpBarY + barHeight);
                }
            }
        }

        void RenderMiniMobBars() override
        {
        }

        void Clear() override
        {
            m_targetVid = std::nullopt;
            m_currentHp = 0;
            m_maxHp = 0;
        }

    private:
        std::optional<EterBase::EntityId> m_targetVid;
        uint32_t m_currentHp{0};
        uint32_t m_maxHp{0};
    };
}
