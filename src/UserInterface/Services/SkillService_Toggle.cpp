#include "../StdAfx.h"
#include "ISkillService.h"
#include "EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "../Core/Events/SkillEvents.h"
#include "../Packet.h"
#include <unordered_map>
#include <vector>

namespace UserInterface::Services
{
    /**
     * @brief Implementacja serwisu zarzadzania umiejetnosciami z obsluga ToggleSkill.
     * Zaprojektowana pod katem C++23, SRP, Zero-Conflict i bez monolitycznych zaleznosci.
     */
    class SkillService_Toggle final : public ISkillService
    {
    public:
        SkillService_Toggle() = default;

        ~SkillService_Toggle() override
        {
            if (networkSubId_ != 0)
            {
                Core::EventBus::GetInstance().Unsubscribe<Core::NetworkPacketReceivedEvent>(networkSubId_);
            }
        }

        void Initialize()
        {
            networkSubId_ = Core::EventBus::GetInstance().Subscribe<Core::NetworkPacketReceivedEvent>(
                [this](const Core::NetworkPacketReceivedEvent& event) {
                    this->OnNetworkPacket(event);
                });
        }

        void SetSkillLevel(EterBase::SkillId id, uint8_t level, uint8_t masterType) override
        {
            auto& skill = skills_[id];
            skill.skillId = id;
            skill.level = level;
            skill.masterType = masterType;

            EterBase::ModernLogger::Info(
                "SkillService: SetSkillLevel for skill {} (level {}, master {})", 
                id.value(), level, masterType);
        }

        void SetSkillCooltime(EterBase::SkillId id, float duration) override
        {
            auto& skill = skills_[id];
            skill.cooltimeRemaining = duration;
            skill.totalCooltime = duration;
            
            EterBase::ModernLogger::Debug(
                "SkillService: Cooldown set for skill {} ({}s)", 
                id.value(), duration);
        }

        bool IsSkillCooltime(EterBase::SkillId id) const override
        {
            auto it = skills_.find(id);
            return (it != skills_.end() && it->second.cooltimeRemaining > 0.0f);
        }

        float GetSkillCooltimeRemaining(EterBase::SkillId id) const override
        {
            auto it = skills_.find(id);
            return (it != skills_.end()) ? it->second.cooltimeRemaining : 0.0f;
        }

        std::optional<PlayerSkillView> GetSkill(EterBase::SkillId id) const override
        {
            auto it = skills_.find(id);
            if (it != skills_.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        void UpdateCooltimes(float deltaTime) override
        {
            for (auto& [id, skill] : skills_)
            {
                if (skill.cooltimeRemaining > 0.0f)
                {
                    skill.cooltimeRemaining -= deltaTime;
                    if (skill.cooltimeRemaining <= 0.0f)
                    {
                        skill.cooltimeRemaining = 0.0f;
                        EterBase::ModernLogger::Debug(
                            "SkillService: Cooldown ended for skill {}", id.value());
                    }
                }
            }
        }

        void Clear() override
        {
            skills_.clear();
            EterBase::ModernLogger::Info("SkillService: All skills cleared.");
        }

    private:
        void OnNetworkPacket(const Core::NetworkPacketReceivedEvent& event)
        {
            if (event.header == GC::SKILL_COOLTIME_END)
            {
                if (event.payload.size() >= sizeof(TPacketGCSkillCoolTimeEnd))
                {
                    const auto* packet = reinterpret_cast<const TPacketGCSkillCoolTimeEnd*>(event.payload.data());
                    EterBase::SkillId skillId{packet->bSkill};
                    
                    auto it = skills_.find(skillId);
                    if (it != skills_.end())
                    {
                        it->second.cooltimeRemaining = 0.0f;
                        EterBase::ModernLogger::Info(
                            "SkillService: Cooldown force ended for skill {} via network packet.", skillId.value());
                    }
                }
            }
        }

        /**
         * @brief Wewnetrzna funkcja do zarzadzania stanem Toggle.
         */
        void ToggleSkillState(EterBase::SkillId id, bool active)
        {
            auto& skill = skills_[id];
            skill.skillId = id;

            if (skill.isActived != active)
            {
                skill.isActived = active;
                EterBase::ModernLogger::Info(
                    "SkillService: ToggleSkill state changed for skill {} to {}", 
                    id.value(), active ? "ON" : "OFF");
                
                auto eventResult = Core::Events::SkillToggleStateChanged::Create(id, active);
                if (eventResult)
                {
                    Core::EventBus::GetInstance().Publish(eventResult.value());
                }
                else
                {
                    EterBase::ModernLogger::Error(
                        "SkillService: Failed to create ToggleEvent for skill {}", id.value());
                }
            }
        }

        std::unordered_map<EterBase::SkillId, PlayerSkillView> skills_;
        uint32_t networkSubId_{0};
    };
}
