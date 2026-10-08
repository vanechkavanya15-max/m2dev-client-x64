#include "SkillDomain.h"
#include "Core/EventBus.h"
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
            if (initialLevel >= 40) {
                newState.masterType = static_cast<uint8_t>(SkillMasterType::PerfectMaster);
                newState.masteryLevel = static_cast<uint8_t>(initialLevel - 39);
            } else if (initialLevel >= 30) {
                newState.masterType = static_cast<uint8_t>(SkillMasterType::GrandMaster);
                newState.masteryLevel = static_cast<uint8_t>(initialLevel - 29);
            } else if (initialLevel >= 20) {
                newState.masterType = static_cast<uint8_t>(SkillMasterType::Master);
                newState.masteryLevel = static_cast<uint8_t>(initialLevel - 19);
            } else {
                newState.masterType = static_cast<uint8_t>(SkillMasterType::Normal);
                newState.masteryLevel = initialLevel;
            }
            newState.cooldownEndTime = std::chrono::steady_clock::now();
            newState.isCoolingDown = false;
            newState.isToggledOn = false;
            m_skills[skillId] = std::move(newState);
        }
    }

    void SkillDomain::RegisterSkill(SkillId skillId, SkillLevel level, uint8_t masterType, uint8_t masteryLevel) {
        std::unique_lock lock(m_mutex);
        SkillState newState;
        newState.level = level;
        newState.masterType = masterType;
        newState.masteryLevel = masteryLevel;
        newState.cooldownEndTime = std::chrono::steady_clock::now();
        newState.isCoolingDown = false;
        newState.isToggledOn = false;
        m_skills[skillId] = std::move(newState);
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
        return 0; // Domyslnie brak umiejetnosci
    }

    void SkillDomain::SetSkillLevel(SkillId skillId, SkillLevel level) {
        std::unique_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            it->second.level = level;
            if (level >= 40) {
                it->second.masterType = static_cast<uint8_t>(SkillMasterType::PerfectMaster);
                it->second.masteryLevel = static_cast<uint8_t>(level - 39);
            } else if (level >= 30) {
                it->second.masterType = static_cast<uint8_t>(SkillMasterType::GrandMaster);
                it->second.masteryLevel = static_cast<uint8_t>(level - 29);
            } else if (level >= 20) {
                it->second.masterType = static_cast<uint8_t>(SkillMasterType::Master);
                it->second.masteryLevel = static_cast<uint8_t>(level - 19);
            } else {
                it->second.masterType = static_cast<uint8_t>(SkillMasterType::Normal);
                it->second.masteryLevel = level;
            }

            // Jesli ustawiamy poziom na 0, wymuszamy reset toggli i wyczyszczenie cooldownu
            if (level == 0) {
                it->second.isToggledOn = false;
                it->second.isCoolingDown = false;
                m_cooldownTracker.ResetCooldown(skillId);
            }
        }
    }

    void SkillDomain::SetSkillMastery(SkillId skillId, uint8_t masterType, uint8_t masteryLevel) {
        std::unique_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            it->second.masterType = masterType;
            it->second.masteryLevel = masteryLevel;
            switch (masterType) {
                case 0:
                    it->second.level = masteryLevel;
                    break;
                case 1:
                    it->second.level = static_cast<SkillLevel>(19 + masteryLevel);
                    break;
                case 2:
                    it->second.level = static_cast<SkillLevel>(29 + masteryLevel);
                    break;
                case 3:
                    it->second.level = static_cast<SkillLevel>(39 + masteryLevel);
                    break;
                default:
                    it->second.level = masteryLevel;
                    break;
            }

            if (it->second.level == 0) {
                it->second.isToggledOn = false;
                it->second.isCoolingDown = false;
                m_cooldownTracker.ResetCooldown(skillId);
            }
        }
    }

    uint8_t SkillDomain::GetSkillMasterType(SkillId skillId) const {
        std::shared_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            return it->second.masterType;
        }
        return 0;
    }

    uint8_t SkillDomain::GetSkillMasteryLevel(SkillId skillId) const {
        std::shared_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            return it->second.masteryLevel;
        }
        return 0;
    }

    const SkillState* SkillDomain::GetSkillState(SkillId skillId) const {
        std::shared_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            return &it->second;
        }
        return nullptr;
    }

    void SkillDomain::StartCooldown(SkillId skillId, std::chrono::milliseconds duration) {
        {
            std::unique_lock lock(m_mutex);
            auto it = m_skills.find(skillId);
            if (it != m_skills.end()) {
                it->second.isCoolingDown = true;
                it->second.cooldownEndTime = std::chrono::steady_clock::now() + duration;
            }
            m_cooldownTracker.StartCooldown(skillId, duration);
        }
        ::Client::Core::SkillCooldownStartedEvent ev(skillId, static_cast<uint32_t>(duration.count()));
        ::UserInterface::Core::EventBus::GetInstance().Publish(ev);
    }

    void SkillDomain::StartCooldown(SkillId skillId, uint32_t durationMs) {
        StartCooldown(skillId, std::chrono::milliseconds(durationMs));
    }

    bool SkillDomain::IsSkillReady(SkillId skillId) const {
        std::shared_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            if (!it->second.isCoolingDown) {
                return true;
            }
            if (std::chrono::steady_clock::now() >= it->second.cooldownEndTime) {
                return true;
            }
            return !m_cooldownTracker.IsOnCooldown(skillId);
        }
        return false;
    }

    void SkillDomain::UpdateCooldowns() {
        std::unique_lock lock(m_mutex);
        auto now = std::chrono::steady_clock::now();
        for (auto& [id, state] : m_skills) {
            if (state.isCoolingDown && (now >= state.cooldownEndTime || !m_cooldownTracker.IsOnCooldown(id))) {
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
            return m_cooldownTracker.GetRemainingCooldown(skillId, now);
        }
        return std::chrono::milliseconds(0);
    }

    uint32_t SkillDomain::GetRemainingCooldownMs(SkillId skillId) const {
        return static_cast<uint32_t>(GetRemainingCooldown(skillId).count());
    }

    float SkillDomain::GetCooldownProgress(SkillId skillId) const {
        return m_cooldownTracker.GetCooldownProgress(skillId);
    }

    void SkillDomain::ResetCooldown(SkillId skillId) {
        std::unique_lock lock(m_mutex);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            it->second.isCoolingDown = false;
            it->second.cooldownEndTime = std::chrono::steady_clock::now();
        }
        m_cooldownTracker.ResetCooldown(skillId);
    }

    void SkillDomain::ResetAllCooldowns() {
        std::unique_lock lock(m_mutex);
        auto now = std::chrono::steady_clock::now();
        for (auto& [id, state] : m_skills) {
            state.isCoolingDown = false;
            state.cooldownEndTime = now;
        }
        m_cooldownTracker.ResetAll();
    }

    void SkillDomain::Clear() noexcept {
        std::unique_lock lock(m_mutex);
        m_skills.clear();
        m_cooldownTracker.ResetAll();
    }

    uint32_t SkillDomain::CalculateSPCost(SkillId skillId) const {
        std::shared_lock lock(m_mutex);
        auto dataIt = m_skillDefinitions.find(skillId);
        if (dataIt == m_skillDefinitions.end()) {
            return 0; // Brak danych definicji oznacza, ze umiejetnosc technicznie nie istnieje
        }
        
        const auto& data = dataIt->second;
        auto it = m_skills.find(skillId);
        auto level = (it != m_skills.end()) ? it->second.level : 0;
        uint8_t masterType = (it != m_skills.end()) ? it->second.masterType : 0;
        
        if (level == 0) {
            return 0; // Brak umiejetnosci to rowniez brak kosztu Many
        }
        
        // Modyfikator skali, implementacja zgodna z podzialem progow ewolucyjnych postaci:
        // Poziom Normal: 1-19 (MasteryMultiplier = 1.0f)
        // Poziom Master (M): 20-29 (MasteryMultiplier = 1.5f)
        // Poziom GrandMaster (G): 30-39 (MasteryMultiplier = 2.0f)
        // Poziom PerfectMaster (P): 40+ (MasteryMultiplier = 2.5f)
        float masteryMultiplier = 1.0f;
        
        if (masterType == 3 || level >= 40) {
            masteryMultiplier = 2.5f;
        } else if (masterType == 2 || level >= 30) {
            masteryMultiplier = 2.0f;
        } else if (masterType == 1 || level >= 20) {
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
            // Wylacznie umiejetnosci oznaczone jako Toggle moga zmieniac ten status (np. Aura Miecza, Silne Cialo)
            if (dataIt != m_skillDefinitions.end() && dataIt->second.type == SkillType::Toggle) {
                // Jesli umiejetnosc zostala oznaczona jako "wlaczona", upewnijmy sie, ze gracz ma poziom wiekszy od 0
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

    Client::Core::Result<void, Client::Core::SkillError> SkillDomain::CanCast(SkillId skillId, std::optional<uint32_t> currentSP) const {
        if (!HasSkill(skillId)) {
            return std::unexpected(Client::Core::SkillError::SkillNotFound);
        }
        if (!IsSkillReady(skillId)) {
            return std::unexpected(Client::Core::SkillError::OnCooldown);
        }
        if (currentSP.has_value()) {
            uint32_t spCost = CalculateSPCost(skillId);
            if (*currentSP < spCost) {
                return std::unexpected(Client::Core::SkillError::NotEnoughSP);
            }
        }
        return {};
    }

    Client::Core::Result<void, Client::Core::SkillError> SkillDomain::CanCast(SkillId skillId, uint32_t currentSP) const {
        return CanCast(skillId, std::optional<uint32_t>{currentSP});
    }

    Client::Core::Result<void, Client::Core::SkillError> SkillDomain::CanUseSkill(SkillId skillId, uint32_t currentSP) const {
        return CanCast(skillId, currentSP);
    }

    Client::Core::Result<void, Client::Core::SkillError> SkillDomain::UseSkill(SkillId skillId, uint32_t currentSP, uint32_t cooldownMs) {
        auto check = CanUseSkill(skillId, currentSP);
        if (!check) {
            return check;
        }

        const auto* data = GetSkillData(skillId);
        if (data && data->type == SkillType::Toggle) {
            ToggleSkill(skillId, !IsSkillToggledOn(skillId));
        }

        if (cooldownMs > 0) {
            StartCooldown(skillId, cooldownMs);
        }

        return {};
    }

    Client::Core::Result<void, Client::Core::SkillError> SkillDomain::StartCooldownResult(SkillId skillId, uint32_t durationMs) {
        if (!HasSkill(skillId)) {
            return std::unexpected(Client::Core::SkillError::SkillNotFound);
        }
        StartCooldown(skillId, durationMs);
        return {};
    }

    Client::Core::Result<uint32_t, Client::Core::SkillError> SkillDomain::CalculateSPCostResult(SkillId skillId) const {
        if (!HasSkill(skillId)) {
            return std::unexpected(Client::Core::SkillError::SkillNotFound);
        }
        return CalculateSPCost(skillId);
    }

    Client::Core::Result<void, Client::Core::SkillError> SkillDomain::SetSkillLevelResult(SkillId skillId, SkillLevel level) {
        if (!HasSkill(skillId)) {
            return std::unexpected(Client::Core::SkillError::SkillNotFound);
        }
        SetSkillLevel(skillId, level);
        return {};
    }

    Client::Core::Result<void, Client::Core::SkillError> SkillDomain::ToggleSkillResult(SkillId skillId, bool state) {
        if (!HasSkill(skillId)) {
            return std::unexpected(Client::Core::SkillError::SkillNotFound);
        }
        const auto* data = GetSkillData(skillId);
        if (!data || data->type != SkillType::Toggle) {
            return std::unexpected(Client::Core::SkillError::RequirementNotMet);
        }
        ToggleSkill(skillId, state);
        return {};
    }

} // namespace Client::Gameplay

