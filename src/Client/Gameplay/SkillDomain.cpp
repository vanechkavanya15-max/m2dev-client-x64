#include "SkillDomain.h"
#include <algorithm>
#include <mutex>

namespace Client::Gameplay {

    void SkillDomain::DefineSkill(SkillId id, const SkillData& data) {
        std::unique_lock lock(m_mutex);
        m_skillDefinitions[id] = data;
    }

    void SkillDomain::RegisterSkill(SkillId skillId, SkillLevel initialLevel) {
        std::unique_lock lock(m_mutex);
        if (!m_skills.contains(skillId)) {
            SkillState newState;
            newState.level = initialLevel;
            newState.cooldownEndTime = std::chrono::steady_clock::now();
            newState.isCoolingDown = false;
            newState.isToggledOn = false;
            m_skills[skillId] = std::move(newState);
        }
    }

    bool SkillDomain::HasSkill(SkillId skillId) const {
        std::shared_lock lock(m_mutex);
        return m_skills.contains(skillId);
    }

    SkillLevel SkillDomain::GetSkillLevel(SkillId skillId) const {
        std::shared_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            return it->second.level;
        }
        return 0; // Domyślnie brak umiejętności
    }

    void SkillDomain::SetSkillLevel(SkillId skillId, SkillLevel level) {
        std::unique_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            it->second.level = level;
            
            // Jeśli ustawiamy poziom na 0, potencjalnie wymuszamy reset toggli i wyczyszczenie cooldownu
            if (level == 0) {
                it->second.isToggledOn = false;
                it->second.isCoolingDown = false;
            }
        }
    }

    void SkillDomain::StartCooldown(SkillId skillId, std::chrono::milliseconds duration) {
        std::unique_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            it->second.isCoolingDown = true;
            // Absolutny czas do momentu odnowienia umiejętności
            it->second.cooldownEndTime = std::chrono::steady_clock::now() + duration;
        }
    }

    bool SkillDomain::IsSkillReady(SkillId skillId) const {
        std::shared_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            // Jeżeli nie ma flagi odnawiania, od razu gotowe
            if (!it->second.isCoolingDown) return true;
            // Jeżeli zegar odliczający jest w przeszłości (lub to moment równy now)
            return std::chrono::steady_clock::now() >= it->second.cooldownEndTime;
        }
        return false;
    }

    void SkillDomain::UpdateCooldowns() {
        std::unique_lock lock(m_mutex);
        auto now = std::chrono::steady_clock::now();
        // Sprawdzamy każdy aktywny zegar i flagę, resetując jeśli czas upłynął
        for (auto& [id, state] : m_skills) {
            if (state.isCoolingDown && now >= state.cooldownEndTime) {
                state.isCoolingDown = false;
            }
        }
    }

    std::chrono::milliseconds SkillDomain::GetRemainingCooldown(SkillId skillId) const {
        std::shared_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end() && it->second.isCoolingDown) {
            auto now = std::chrono::steady_clock::now();
            if (now < it->second.cooldownEndTime) {
                return std::chrono::duration_cast<std::chrono::milliseconds>(it->second.cooldownEndTime - now);
            }
        }
        return std::chrono::milliseconds(0);
    }

    void SkillDomain::ResetCooldown(SkillId skillId) {
        std::unique_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            it->second.isCoolingDown = false;
            it->second.cooldownEndTime = std::chrono::steady_clock::now();
        }
    }

    uint32_t SkillDomain::CalculateSPCost(SkillId skillId) const {
        std::shared_lock lock(m_mutex);
        auto dataIt = m_skillDefinitions.find(skillId);
        if (dataIt == m_skillDefinitions.end()) {
            return 0; // Brak danych definicji oznacza, że umiejętność technicznie nie istnieje jako obiekt zużywający punkty.
        }
        
        const auto& data = dataIt->second;
        auto it = m_skills.find(skillId);
        auto level = (it != m_skills.end()) ? it->second.level : 0;
        
        if (level == 0) {
            return 0; // Brak umiejętności to również brak kosztu Many.
        }
        
        // Modyfikator skali, implementacja zgodna z podziałem progów ewolucyjnych postaci:
        // Poziom Normal: 1-19 (MasteryMultiplier = 1.0f)
        // Poziom Master (M): 20-29 (MasteryMultiplier = 1.5f)
        // Poziom GrandMaster (G): 30-39 (MasteryMultiplier = 2.0f)
        // Poziom PerfectMaster (P): 40+ (MasteryMultiplier = 2.5f)
        float masteryMultiplier = 1.0f;
        
        if (level >= 40) {
            masteryMultiplier = 2.5f;
        } else if (level >= 30) {
            masteryMultiplier = 2.0f;
        } else if (level >= 20) {
            masteryMultiplier = 1.5f;
        }

        float computedCost = data.baseSPCost + (level * data.spMultiplier * masteryMultiplier);
        
        return static_cast<uint32_t>(computedCost);
    }

    void SkillDomain::ToggleSkill(SkillId skillId, bool state) {
        std::unique_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            auto dataIt = m_skillDefinitions.find(skillId);
            // Wyłącznie umiejętności oznaczone jako Toggle mogą zmieniać ten status (np. Aura Miecza, Silne Ciało).
            if (dataIt != m_skillDefinitions.end() && dataIt->second.type == SkillType::Toggle) {
                // Jeśli umiejętność została oznaczona jako "włączona", upewnijmy się, że gracz ma poziom większy od 0.
                if (state == true && it->second.level == 0) {
                    it->second.isToggledOn = false;
                    return;
                }
                
                it->second.isToggledOn = state;
            }
        }
    }

    bool SkillDomain::IsSkillToggledOn(SkillId skillId) const {
        std::shared_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            return it->second.isToggledOn;
        }
        return false;
    }

    const SkillData* SkillDomain::GetSkillData(SkillId skillId) const {
        std::shared_lock lock(m_mutex);
        auto it = m_skillDefinitions.find(skillId);
        if (it != m_skillDefinitions.end()) {
            return &it->second;
        }
        return nullptr;
    }

} // namespace Client::Gameplay
