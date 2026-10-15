#include "../StdAfx.h"
#include "ITargetHpBarService.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../Core/EventBus.h"
#include "../Core/Events.h"

#include <unordered_map>
#include <memory>
#include <chrono>
#include <expected>
#include <algorithm>

namespace UserInterface::TextTail
{
    /**
     * @brief Custom event emitted for GUI to render the smooth delayed red bar.
     * Defined strictly here to abide by Zero-Conflict rules (no header modification).
     */
    struct TargetHpDelayedUpdateEvent : public Core::IEvent
    {
        uint32_t targetId;
        uint32_t currentHp;
        uint32_t delayedHp;
        uint32_t maxHp;

        TargetHpDelayedUpdateEvent(uint32_t tid = 0, uint32_t cur = 0, uint32_t del = 0, uint32_t max = 0)
            : targetId(tid), currentHp(cur), delayedHp(del), maxHp(max) {}
    };

    /**
     * @brief Struktura przechowujaca stan HP celu dla plynnej animacji.
     */
    struct TargetHpState
    {
        uint32_t currentHp{0};
        uint32_t delayedHp{0};
        uint32_t maxHp{0};
        std::chrono::steady_clock::time_point lastUpdateTime;
    };

    /**
     * @brief Specyficzna dla akcji klasa sluzaca do uaktualnienia HP celu
     * z opoznionym czerwonym slupkiem obrazen (Action-Specific Class).
     */
    class TargetHpBarService_Update final : public ITargetHpBarService
    {
    public:
        TargetHpBarService_Update() = default;
        ~TargetHpBarService_Update() override = default;

        void ShowTargetBar(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) override {}

        void UpdateTargetHp(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) override
        {
            auto result = ProcessTargetHpUpdate(vid, currentHp, maxHp);
            if (!result.has_value()) {
                EterBase::ModernLogger::Error("TargetHpBarService_Update: Failed to update target HP: {}", EterBase::ToString(result.error()));
            }
        }

        void HideTargetBar() override {}

        /**
         * @brief Oblicza i publikuje plynna animacje opoznionego slupka obrazen (delayed red bar).
         */
        void RenderTargetBar() override
        {
            auto now = std::chrono::steady_clock::now();
            for (auto& [vid, state] : m_targetStates)
            {
                if (state.delayedHp > state.currentHp)
                {
                    auto dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - state.lastUpdateTime).count();
                    if (dt_ms > 0)
                    {
                        // Spadek rzedu 10% max HP na sekunde
                        double dropFactor = (static_cast<double>(dt_ms) / 1000.0) * 0.1;
                        uint32_t dropRate = std::max<uint32_t>(1, static_cast<uint32_t>(state.maxHp * dropFactor));
                        
                        if (state.delayedHp > state.currentHp + dropRate)
                            state.delayedHp -= dropRate;
                        else
                            state.delayedHp = state.currentHp;
                            
                        state.lastUpdateTime = now;

                        // Emituj customowy event z opoznionym HP do wlasciwego kodu renderujacego w PythonUI
                        TargetHpDelayedUpdateEvent ev{vid.value(), state.currentHp, state.delayedHp, state.maxHp};
                        UserInterface::Core::EventBus::GetInstance().Publish(ev);
                    }
                }
            }
        }

        void RenderMiniMobBars() override {}
        
        void Clear() override 
        {
            m_targetStates.clear();
            EterBase::ModernLogger::Debug("TargetHpBarService_Update: Wyczyszczono stany HP celow.");
        }

    private:
        /**
         * @brief Core logic enforcing C++23 return type constraint.
         */
        EterBase::PacketResult<void> ProcessTargetHpUpdate(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp)
        {
            if (!vid)
            {
                EterBase::ModernLogger::Error("TargetHpBarService_Update: Proba aktualizacji HP dla niewaznego EntityId (0)");
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            auto now = std::chrono::steady_clock::now();
            auto& state = m_targetStates[vid];

            if (state.maxHp == 0 || state.maxHp != maxHp)
            {
                state.maxHp = maxHp;
                state.delayedHp = currentHp; 
            }
            else if (currentHp < state.currentHp)
            {
                state.delayedHp = state.currentHp;
            }

            state.currentHp = currentHp;
            state.lastUpdateTime = now;

            EterBase::ModernLogger::Debug("TargetHpBarService_Update: Zaktualizowano HP celu {}: {}/{}", 
                vid.value(), currentHp, maxHp);

            // Emit podstawowego zdarzenia ubytku HP
            ::Core::Events::HpChange ev;
            ev.targetId = vid.value();
            ev.currentHp = currentHp;
            ev.maxHp = maxHp;
            UserInterface::Core::EventBus::GetInstance().Publish(ev);

            return {};
        }

        std::unordered_map<EterBase::EntityId, TargetHpState> m_targetStates;
    };

    // ====================================================================
    // Factory method for creating instances of the service
    // ====================================================================
    std::unique_ptr<ITargetHpBarService> CreateTargetHpBarService_Update()
    {
        return std::make_unique<TargetHpBarService_Update>();
    }
}
