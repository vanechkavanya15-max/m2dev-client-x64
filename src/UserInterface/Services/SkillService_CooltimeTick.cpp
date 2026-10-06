#include "../StdAfx.h"
#include "ISkillService.h"
#include "Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

#include <unordered_map>
#include <algorithm>
#include <vector>

namespace UserInterface::Services
{

    /**
     * @brief Zdarzenie emitowane po aktualizacji (tiknieciu) czasu odnowienia umiejetnosci.
     * Zgodnie z zasada zero-conflict definiujemy je lokalnie.
     */
    struct SkillCooltimeUpdatedEvent : public Core::IEvent
    {
        EterBase::SkillId skillId;
        float cooltimeRemaining;
        float totalCooltime;

        explicit SkillCooltimeUpdatedEvent(EterBase::SkillId skillId, float remaining, float total)
            : skillId(skillId), cooltimeRemaining(remaining), totalCooltime(total) {}
    };

    /**
     * @brief Implementacja serwisu umiejetnosci ograniczona do obslugi tikow czasu odnowienia.
     * Zgodnie z zasada SRP, serwis skupia sie na logice domenowej bez bezposredniego wolania GUI.
     */
    class SkillService final : public ISkillService
    {
    public:
        SkillService() = default;
        ~SkillService() override = default;

        void SetSkillLevel(EterBase::SkillId id, uint8_t level, uint8_t masterType) override
        {
            auto& skill = GetOrAddSkill(id);
            skill.level = level;
            skill.masterType = masterType;
            EterBase::ModernLogger::Debug("SkillService: Zaktualizowano poziom umiejetnosci {}.", id.value());
        }

        void SetSkillCooltime(EterBase::SkillId id, float duration) override
        {
            auto& skill = GetOrAddSkill(id);
            skill.totalCooltime = duration;
            skill.cooltimeRemaining = duration;

            EterBase::ModernLogger::Info("SkillService: Ustawiono cooltime dla umiejetnosci {} na {}s.", id.value(), duration);
            
            Core::EventBus::GetInstance().Publish(
                SkillCooltimeUpdatedEvent(id, duration, duration)
            );
        }

        bool IsSkillCooltime(EterBase::SkillId id) const override
        {
            auto it = m_skills.find(id);
            if (it != m_skills.end())
            {
                return it->second.cooltimeRemaining > 0.0f;
            }
            return false;
        }

        float GetSkillCooltimeRemaining(EterBase::SkillId id) const override
        {
            auto it = m_skills.find(id);
            if (it != m_skills.end())
            {
                return it->second.cooltimeRemaining;
            }
            return 0.0f;
        }

        std::optional<PlayerSkillView> GetSkill(EterBase::SkillId id) const override
        {
            auto it = m_skills.find(id);
            if (it != m_skills.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        /**
         * @brief Aktualizacja wszystkich czasow odnowienia plynąca z deltaTime.
         */
        void UpdateCooltimes(float deltaTime) override
        {
            if (deltaTime <= 0.0f)
            {
                return;
            }

            // Uzywamy wektora do przechowania zdarzen, aby uniknac blokowania i ODR
            std::vector<SkillCooltimeUpdatedEvent> eventsToPublish;

            for (auto& [id, skill] : m_skills)
            {
                if (skill.cooltimeRemaining > 0.0f)
                {
                    skill.cooltimeRemaining -= deltaTime;
                    
                    if (skill.cooltimeRemaining < 0.0f)
                    {
                        skill.cooltimeRemaining = 0.0f;
                    }

                    // Przygotowujemy event do wyslania
                    eventsToPublish.emplace_back(id, skill.cooltimeRemaining, skill.totalCooltime);
                }
            }

            // Publikujemy zdarzenia poza glowna iteracja
            for (const auto& event : eventsToPublish)
            {
                Core::EventBus::GetInstance().Publish(event);
            }
        }

        void Clear() override
        {
            m_skills.clear();
            EterBase::ModernLogger::Debug("SkillService: Wyczyszczono stan umiejetnosci gracza.");
        }

    private:
        PlayerSkillView& GetOrAddSkill(EterBase::SkillId id)
        {
            auto it = m_skills.find(id);
            if (it == m_skills.end())
            {
                PlayerSkillView newSkill;
                newSkill.skillId = id;
                newSkill.level = 0;
                newSkill.masterType = 0;
                newSkill.cooltimeRemaining = 0.0f;
                newSkill.totalCooltime = 0.0f;
                newSkill.isActived = false;

                auto [insertedIt, success] = m_skills.emplace(id, newSkill);
                return insertedIt->second;
            }
            return it->second;
        }

        std::unordered_map<EterBase::SkillId, PlayerSkillView> m_skills;
    };
}
