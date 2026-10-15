#include "../StdAfx.h"
#include "ISkillService.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"

#include <unordered_map>
#include <mutex>
#include <memory>
#include <optional>
#include <expected>

namespace UserInterface::Services
{
    /**
     * @brief Zdarzenie aktualizacji poziomu umiejetnosci konia.
     */
    struct HorseSkillLevelChangedEvent : public Core::IEvent
    {
        EterBase::SkillId skillId;
        uint8_t level;
        uint8_t masterType;

        HorseSkillLevelChangedEvent(EterBase::SkillId id, uint8_t lvl, uint8_t type)
            : skillId(id), level(lvl), masterType(type) {}
    };

    /**
     * @brief Zdarzenie zmiany cooldownu umiejetnosci konnej.
     */
    struct HorseSkillCooltimeChangedEvent : public Core::IEvent
    {
        EterBase::SkillId skillId;
        float remaining;
        float total;

        HorseSkillCooltimeChangedEvent(EterBase::SkillId id, float rem, float tot)
            : skillId(id), remaining(rem), total(tot) {}
    };

    /**
     * @brief Implementacja mikro-serwisu obslugi umiejetnosci konnych.
     */
    class SkillService_Horse final : public ISkillService
    {
    public:
        SkillService_Horse() = default;
        ~SkillService_Horse() override = default;

        void SetSkillLevel(EterBase::SkillId id, uint8_t level, uint8_t masterType) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            auto& skill = m_skills[id];
            skill.skillId = id;
            skill.level = level;
            skill.masterType = masterType;

            EterBase::ModernLogger::Debug(
                "SkillService_Horse: Zaktualizowano poziom skilla konnego {} (poziom: {}, master: {})",
                id.value(), level, masterType);

            Core::EventBus::GetInstance().Publish(HorseSkillLevelChangedEvent{id, level, masterType});
        }

        void SetSkillCooltime(EterBase::SkillId id, float duration) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            auto& skill = m_skills[id];
            skill.skillId = id;
            skill.totalCooltime = duration;
            skill.cooltimeRemaining = duration;

            EterBase::ModernLogger::Debug(
                "SkillService_Horse: Ustawiono cooltime skilla konnego {} na {:.2f}s",
                id.value(), duration);

            Core::EventBus::GetInstance().Publish(HorseSkillCooltimeChangedEvent{id, skill.cooltimeRemaining, skill.totalCooltime});
        }

        [[nodiscard]] bool IsSkillCooltime(EterBase::SkillId id) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (auto it = m_skills.find(id); it != m_skills.end())
            {
                return it->second.cooltimeRemaining > 0.0f;
            }
            return false;
        }

        [[nodiscard]] float GetSkillCooltimeRemaining(EterBase::SkillId id) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (auto it = m_skills.find(id); it != m_skills.end())
            {
                return it->second.cooltimeRemaining;
            }
            return 0.0f;
        }

        [[nodiscard]] std::optional<PlayerSkillView> GetSkill(EterBase::SkillId id) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (auto it = m_skills.find(id); it != m_skills.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        void UpdateCooltimes(float deltaTime) override
        {
            if (deltaTime <= 0.0f)
                return;

            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& [id, skill] : m_skills)
            {
                if (skill.cooltimeRemaining > 0.0f)
                {
                    skill.cooltimeRemaining -= deltaTime;
                    if (skill.cooltimeRemaining <= 0.0f)
                    {
                        skill.cooltimeRemaining = 0.0f;
                        EterBase::ModernLogger::Debug(
                            "SkillService_Horse: Cooltime skilla konnego {} zakonczony", id.value());
                        
                        Core::EventBus::GetInstance().Publish(HorseSkillCooltimeChangedEvent{id, 0.0f, skill.totalCooltime});
                    }
                }
            }
        }

        void Clear() override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_skills.clear();
            EterBase::ModernLogger::Debug("SkillService_Horse: Wyczyszczono stan skilli konnych");
        }

        /**
         * @brief Weryfikuje czy dany skill moze byc uzyty opierajac sie o poziom konia, 
         * i zwraca modern result C++23.
         */
        template <typename T = void>
        using CombatResult = std::expected<T, EterBase::CombatError>;

        [[nodiscard]] CombatResult<void> ValidateHorseSkillUsage(EterBase::SkillId skillId, uint8_t horseLevel) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            // Kon bojowy (poziom >= 11) - minimalny wymóg do skilli bojowych
            if (horseLevel < 11)
            {
                return EterBase::MakeError(EterBase::CombatError::InvalidAction);
            }

            // Metin2 logika: Skille takie jak ciecie z siodla (id 137 i nowsze) 
            // wymagaja poziomu konia militarnego (20-21).
            if (skillId.value() >= 137 && horseLevel < 20)
            {
                return EterBase::MakeError(EterBase::CombatError::InvalidAction);
            }
            
            if (auto it = m_skills.find(skillId); it != m_skills.end())
            {
                if (it->second.cooltimeRemaining > 0.0f)
                {
                    return EterBase::MakeError(EterBase::CombatError::OnCooldown);
                }
            }
            else
            {
                // Jesli nie znamy skilla
                return EterBase::MakeError(EterBase::CombatError::InvalidAction);
            }

            return {};
        }

    private:
        mutable std::mutex m_mutex;
        std::unordered_map<EterBase::SkillId, PlayerSkillView> m_skills;
    };
    
    // Factory method required to instantiate service (publicly available from this cpp)
    std::unique_ptr<ISkillService> CreateHorseSkillService()
    {
        return std::make_unique<SkillService_Horse>();
    }
}
