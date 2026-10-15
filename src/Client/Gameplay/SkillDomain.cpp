#include "SkillDomain.h"
#include "UserInterface/Core/EventBus.h"
#include <algorithm>
#include <cmath>

namespace Client::Gameplay {

void SkillDomain::DefineSkill(SkillId id, const SkillData& data)
{
    std::unique_lock lock(m_mutex);
    m_skillDefinitions[id] = data;
}

void SkillDomain::RegisterSkill(SkillId skillId, SkillLevel initialLevel)
{
    std::unique_lock lock(m_mutex);
    auto& state = m_skills[skillId];
    state.level = initialLevel;
    if (initialLevel <= 19) {
        state.masterType = 0;
        state.masteryLevel = initialLevel;
    } else if (initialLevel <= 29) {
        state.masterType = 1;
        state.masteryLevel = static_cast<uint8_t>(initialLevel - 19);
    } else if (initialLevel <= 39) {
        state.masterType = 2;
        state.masteryLevel = static_cast<uint8_t>(initialLevel - 29);
    } else {
        state.masterType = 3;
        state.masteryLevel = static_cast<uint8_t>(initialLevel - 39);
    }
}

void SkillDomain::RegisterSkill(SkillId skillId, SkillLevel level, uint8_t masterType, uint8_t masteryLevel)
{
    std::unique_lock lock(m_mutex);
    auto& state = m_skills[skillId];
    state.level = level;
    state.masterType = masterType;
    state.masteryLevel = masteryLevel;
}

bool SkillDomain::HasSkill(SkillId skillId) const
{
    std::shared_lock lock(m_mutex);
    return m_skills.find(skillId) != m_skills.end();
}

SkillLevel SkillDomain::GetSkillLevel(SkillId skillId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_skills.find(skillId);
    return (it != m_skills.end()) ? it->second.level : 0;
}

void SkillDomain::SetSkillLevel(SkillId skillId, SkillLevel level)
{
    std::unique_lock lock(m_mutex);
    auto it = m_skills.find(skillId);
    if (it == m_skills.end()) {
        return;
    }
    it->second.level = level;
    if (level <= 19) {
        it->second.masterType = 0;
        it->second.masteryLevel = level;
    } else if (level <= 29) {
        it->second.masterType = 1;
        it->second.masteryLevel = static_cast<uint8_t>(level - 19);
    } else if (level <= 39) {
        it->second.masterType = 2;
        it->second.masteryLevel = static_cast<uint8_t>(level - 29);
    } else {
        it->second.masterType = 3;
        it->second.masteryLevel = static_cast<uint8_t>(level - 39);
    }
}

void SkillDomain::SetSkillMastery(SkillId skillId, uint8_t masterType, uint8_t masteryLevel)
{
    std::unique_lock lock(m_mutex);
    auto it = m_skills.find(skillId);
    if (it == m_skills.end()) {
        return;
    }
    it->second.masterType = masterType;
    it->second.masteryLevel = masteryLevel;
    if (masterType == 0) {
        it->second.level = masteryLevel;
    } else if (masterType == 1) {
        it->second.level = static_cast<SkillLevel>(19 + masteryLevel);
    } else if (masterType == 2) {
        it->second.level = static_cast<SkillLevel>(29 + masteryLevel);
    } else {
        it->second.level = static_cast<SkillLevel>(39 + masteryLevel);
    }
}

uint8_t SkillDomain::GetSkillMasterType(SkillId skillId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_skills.find(skillId);
    return (it != m_skills.end()) ? it->second.masterType : 0;
}

uint8_t SkillDomain::GetSkillMasteryLevel(SkillId skillId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_skills.find(skillId);
    return (it != m_skills.end()) ? it->second.masteryLevel : 0;
}

const SkillState* SkillDomain::GetSkillState(SkillId skillId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_skills.find(skillId);
    return (it != m_skills.end()) ? &it->second : nullptr;
}

void SkillDomain::StartCooldown(SkillId skillId, std::chrono::milliseconds duration)
{
    uint32_t durationMs = static_cast<uint32_t>(duration.count());
    {
        std::unique_lock lock(m_mutex);
        m_cooldownTracker.StartCooldown(skillId, duration);
        auto it = m_skills.find(skillId);
        if (it != m_skills.end()) {
            it->second.isCoolingDown = true;
            it->second.cooldownEndTime = std::chrono::steady_clock::now() + duration;
        }
    }
    ::UserInterface::Core::EventBus::GetInstance().Publish(
        ::Client::Core::SkillCooldownStartedEvent{ skillId, durationMs }
    );
}

void SkillDomain::StartCooldown(SkillId skillId, uint32_t durationMs)
{
    StartCooldown(skillId, std::chrono::milliseconds(durationMs));
}

bool SkillDomain::IsSkillReady(SkillId skillId) const
{
    std::shared_lock lock(m_mutex);
    return !m_cooldownTracker.IsOnCooldown(skillId);
}

void SkillDomain::UpdateCooldowns()
{
    std::unique_lock lock(m_mutex);
    for (auto& [id, state] : m_skills) {
        if (state.isCoolingDown && !m_cooldownTracker.IsOnCooldown(id)) {
            state.isCoolingDown = false;
        }
    }
}

std::chrono::milliseconds SkillDomain::GetRemainingCooldown(SkillId skillId) const
{
    std::shared_lock lock(m_mutex);
    return m_cooldownTracker.GetRemainingCooldown(skillId);
}

uint32_t SkillDomain::GetRemainingCooldownMs(SkillId skillId) const
{
    std::shared_lock lock(m_mutex);
    return m_cooldownTracker.GetRemainingCooldownMs(skillId);
}

float SkillDomain::GetCooldownProgress(SkillId skillId) const
{
    std::shared_lock lock(m_mutex);
    return m_cooldownTracker.GetCooldownProgress(skillId);
}

void SkillDomain::ResetCooldown(SkillId skillId)
{
    std::unique_lock lock(m_mutex);
    m_cooldownTracker.ResetCooldown(skillId);
    auto it = m_skills.find(skillId);
    if (it != m_skills.end()) {
        it->second.isCoolingDown = false;
    }
}

void SkillDomain::ResetAllCooldowns()
{
    std::unique_lock lock(m_mutex);
    m_cooldownTracker.ResetAll();
    for (auto& [id, state] : m_skills) {
        state.isCoolingDown = false;
    }
}

void SkillDomain::Clear() noexcept
{
    std::unique_lock lock(m_mutex);
    m_skills.clear();
    m_skillDefinitions.clear();
    m_cooldownTracker.ResetAll();
}

uint32_t SkillDomain::CalculateSPCost(SkillId skillId) const
{
    std::shared_lock lock(m_mutex);
    auto itDef = m_skillDefinitions.find(skillId);
    if (itDef == m_skillDefinitions.end()) {
        return 0;
    }
    const auto& def = itDef->second;
    auto itState = m_skills.find(skillId);
    uint8_t level = (itState != m_skills.end()) ? itState->second.level : 0;
    uint8_t masterType = (itState != m_skills.end()) ? itState->second.masterType : 0;

    float mult = 1.0f;
    if (masterType == 1) mult = 1.5f;
    else if (masterType == 2) mult = 2.0f;
    else if (masterType == 3) mult = 2.5f;

    return static_cast<uint32_t>(def.baseSPCost + (level * def.spMultiplier * mult));
}

void SkillDomain::ToggleSkill(SkillId skillId, bool state)
{
    std::unique_lock lock(m_mutex);
    auto itDef = m_skillDefinitions.find(skillId);
    if (itDef != m_skillDefinitions.end() && itDef->second.type != SkillType::Toggle) {
        return;
    }
    auto it = m_skills.find(skillId);
    if (it != m_skills.end()) {
        it->second.isToggledOn = state;
    }
}

bool SkillDomain::IsSkillToggledOn(SkillId skillId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_skills.find(skillId);
    return (it != m_skills.end()) ? it->second.isToggledOn : false;
}

const SkillData* SkillDomain::GetSkillData(SkillId skillId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_skillDefinitions.find(skillId);
    return (it != m_skillDefinitions.end()) ? &it->second : nullptr;
}

SkillDomain::Result<void, SkillDomain::SkillError> SkillDomain::CanCast(SkillId skillId, std::optional<uint32_t> currentSP) const
{
    std::shared_lock lock(m_mutex);
    if (m_skills.find(skillId) == m_skills.end()) {
        return std::unexpected(SkillError::SkillNotFound);
    }
    if (m_cooldownTracker.IsOnCooldown(skillId)) {
        return std::unexpected(SkillError::OnCooldown);
    }
    if (currentSP.has_value()) {
        auto itDef = m_skillDefinitions.find(skillId);
        if (itDef != m_skillDefinitions.end()) {
            const auto& def = itDef->second;
            auto itState = m_skills.find(skillId);
            uint8_t level = (itState != m_skills.end()) ? itState->second.level : 0;
            uint8_t masterType = (itState != m_skills.end()) ? itState->second.masterType : 0;
            float mult = 1.0f;
            if (masterType == 1) mult = 1.5f;
            else if (masterType == 2) mult = 2.0f;
            else if (masterType == 3) mult = 2.5f;
            uint32_t cost = static_cast<uint32_t>(def.baseSPCost + (level * def.spMultiplier * mult));
            if (*currentSP < cost) {
                return std::unexpected(SkillError::NotEnoughSP);
            }
        }
    }
    return {};
}

SkillDomain::Result<void, SkillDomain::SkillError> SkillDomain::CanCast(SkillId skillId, uint32_t currentSP) const
{
    return CanCast(skillId, std::optional<uint32_t>{currentSP});
}

SkillDomain::Result<uint32_t, SkillDomain::SkillError> SkillDomain::CalculateSPCostResult(SkillId skillId) const
{
    std::shared_lock lock(m_mutex);
    auto itDef = m_skillDefinitions.find(skillId);
    if (itDef == m_skillDefinitions.end()) {
        return std::unexpected(SkillError::SkillNotFound);
    }
    const auto& def = itDef->second;
    auto itState = m_skills.find(skillId);
    uint8_t level = (itState != m_skills.end()) ? itState->second.level : 0;
    uint8_t masterType = (itState != m_skills.end()) ? itState->second.masterType : 0;

    float mult = 1.0f;
    if (masterType == 1) mult = 1.5f;
    else if (masterType == 2) mult = 2.0f;
    else if (masterType == 3) mult = 2.5f;

    return static_cast<uint32_t>(def.baseSPCost + (level * def.spMultiplier * mult));
}

SkillDomain::Result<void, SkillDomain::SkillError> SkillDomain::CanUseSkill(SkillId skillId, uint32_t currentSP) const
{
    return CanCast(skillId, currentSP);
}

SkillDomain::Result<void, SkillDomain::SkillError> SkillDomain::UseSkill(SkillId skillId, uint32_t currentSP, uint32_t cooldownMs)
{
    auto canRes = CanCast(skillId, currentSP);
    if (!canRes) {
        return canRes;
    }
    StartCooldown(skillId, cooldownMs);
    return {};
}

SkillDomain::Result<void, SkillDomain::SkillError> SkillDomain::StartCooldownResult(SkillId skillId, uint32_t durationMs)
{
    if (!HasSkill(skillId)) {
        return std::unexpected(SkillError::SkillNotFound);
    }
    StartCooldown(skillId, durationMs);
    return {};
}

SkillDomain::Result<void, SkillDomain::SkillError> SkillDomain::SetSkillLevelResult(SkillId skillId, SkillLevel level)
{
    if (!HasSkill(skillId)) {
        return std::unexpected(SkillError::SkillNotFound);
    }
    SetSkillLevel(skillId, level);
    return {};
}

SkillDomain::Result<void, SkillDomain::SkillError> SkillDomain::ToggleSkillResult(SkillId skillId, bool state)
{
    if (!HasSkill(skillId)) {
        return std::unexpected(SkillError::SkillNotFound);
    }
    {
        std::shared_lock lock(m_mutex);
        auto itDef = m_skillDefinitions.find(skillId);
        if (itDef != m_skillDefinitions.end() && itDef->second.type != SkillType::Toggle) {
            return std::unexpected(SkillError::RequirementNotMet);
        }
    }
    ToggleSkill(skillId, state);
    return {};
}

} // namespace Client::Gameplay
