#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <expected>
#include <memory>
#include <span>
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../Core/EventBus.h"

namespace UserInterface::Domain
{

/**
 * @brief Represents information about a specific guild grade/rank.
 */
struct GuildGradeData
{
    uint8_t gradeNumber;        ///< The numerical ID of the grade (usually 1-15)
    std::string gradeName;      ///< The name of the grade
    uint8_t authorityFlags;     ///< Bitmask of permissions (invite, kick, etc.)
};

/**
 * @brief Core data structure representing a guild member.
 */
struct GuildMemberData
{
    EterBase::EntityId id;              ///< Member's Entity ID (PID)
    std::string name;                   ///< Member's character name
    uint8_t gradeNumber;                ///< Member's grade within the guild
    uint8_t job;                        ///< Member's character class/job
    EterBase::PlayerLevel level;        ///< Member's character level
    uint32_t offerExperience;           ///< Amount of experience donated by the member
    uint8_t generalFlag;                ///< Is member a general (1) or not (0)
};

/**
 * @brief Represents information about a specific guild skill.
 */
struct GuildSkillData
{
    EterBase::SkillId id;       ///< The ID of the skill
    uint8_t level;              ///< Current level of the skill
    uint8_t maxLevel;           ///< Maximum achievable level
    uint8_t point;              ///< Points invested in this skill
};

/**
 * @brief Represents basic information about the guild itself.
 */
struct GuildInfo
{
    EterBase::GuildId id;               ///< The unique ID of the guild
    std::string name;                   ///< The name of the guild
    EterBase::PlayerLevel level;        ///< The current level of the guild
    uint32_t experience;                ///< The current experience of the guild
    uint32_t experienceSummary;         ///< The total accumulated experience
    EterBase::EntityId masterId;        ///< Entity ID of the guild master
    bool hasLand;                       ///< True if the guild owns a land
    uint32_t memberCountMax;            ///< Maximum allowed members
    uint32_t memberLevelAverage;        ///< Average level of all members
};

/**
 * @brief Event published when the guild info is updated.
 */
struct GuildInfoUpdatedEvent : public Core::IEvent
{
    GuildInfo info;
    explicit GuildInfoUpdatedEvent(const GuildInfo& i) : info(i) {}
};

/**
 * @brief Event published when a guild member is added or updated.
 */
struct GuildMemberUpdatedEvent : public Core::IEvent
{
    GuildMemberData member;
    explicit GuildMemberUpdatedEvent(const GuildMemberData& m) : member(m) {}
};

/**
 * @brief Event published when a guild member is removed.
 */
struct GuildMemberRemovedEvent : public Core::IEvent
{
    EterBase::EntityId memberId;
    explicit GuildMemberRemovedEvent(EterBase::EntityId id) : memberId(id) {}
};

/**
 * @brief Error codes specific to Guild Domain logic.
 */
enum class GuildError : uint8_t
{
    None = 0,
    NotAMember,
    MemberNotFound,
    GradeNotFound,
    GuildNotEnabled,
    SkillNotFound
};

[[nodiscard]] constexpr std::string_view ToString(GuildError err) noexcept
{
    switch (err)
    {
        case GuildError::None: return "None";
        case GuildError::NotAMember: return "NotAMember";
        case GuildError::MemberNotFound: return "MemberNotFound";
        case GuildError::GradeNotFound: return "GradeNotFound";
        case GuildError::GuildNotEnabled: return "GuildNotEnabled";
        case GuildError::SkillNotFound: return "SkillNotFound";
    }
    return "UnknownGuildError";
}

} // namespace UserInterface::Domain

template <>
struct std::formatter<UserInterface::Domain::GuildError> : std::formatter<std::string_view>
{
    auto format(UserInterface::Domain::GuildError err, std::format_context& ctx) const
    {
        return std::formatter<std::string_view>::format(UserInterface::Domain::ToString(err), ctx);
    }
};

namespace UserInterface::Domain
{

/**
 * @brief Custom Result type for Guild operations.
 */
template <typename T = void>
using GuildResult = std::expected<T, GuildError>;

/**
 * @brief Manages the Guild domain logic and memory state.
 * 
 * Replaces the old CPythonGuild, decoupling state from the UI.
 * Updates C++ memory state and publishes events via EventBus.
 */
class GuildContainerModel
{
public:
    /**
     * @brief Constructs a new GuildContainerModel.
     */
    GuildContainerModel() : m_isEnabled(false) {}

    /**
     * @brief Enables or disables the guild functionality for the local player.
     * @param enable True to enable, false to disable.
     */
    void SetEnable(bool enable) noexcept
    {
        m_isEnabled = enable;
    }

    /**
     * @brief Checks if the guild functionality is enabled.
     * @return True if enabled, false otherwise.
     */
    [[nodiscard]] bool IsEnabled() const noexcept
    {
        return m_isEnabled;
    }

    /**
     * @brief Sets the basic guild information.
     * @param info The new guild info.
     */
    void SetGuildInfo(const GuildInfo& info)
    {
        m_info = info;
        Core::EventBus::GetInstance().Publish(GuildInfoUpdatedEvent(m_info));
    }

    /**
     * @brief Gets the current guild information.
     * @return A std::optional containing the GuildInfo if the guild is enabled.
     */
    [[nodiscard]] std::optional<GuildInfo> GetGuildInfo() const noexcept
    {
        if (!m_isEnabled)
        {
            return std::nullopt;
        }
        return m_info;
    }

    /**
     * @brief Adds or updates a guild member.
     * @param member The member data.
     * @return GuildResult<void> indicating success.
     */
    GuildResult<void> UpsertMember(const GuildMemberData& member)
    {
        if (!m_isEnabled)
        {
            return std::unexpected(GuildError::GuildNotEnabled);
        }
        
        m_members[member.id] = member;
        Core::EventBus::GetInstance().Publish(GuildMemberUpdatedEvent(member));
        return {};
    }

    /**
     * @brief Removes a guild member by their ID.
     * @param id The Entity ID of the member.
     * @return GuildResult<void> indicating success or error if not found.
     */
    GuildResult<void> RemoveMember(EterBase::EntityId id)
    {
        if (!m_isEnabled)
        {
            return std::unexpected(GuildError::GuildNotEnabled);
        }

        if (m_members.erase(id) == 0)
        {
            return std::unexpected(GuildError::MemberNotFound);
        }
        
        Core::EventBus::GetInstance().Publish(GuildMemberRemovedEvent(id));
        return {};
    }

    /**
     * @brief Retrieves a guild member by their ID.
     * @param id The Entity ID of the member.
     * @return GuildResult<GuildMemberData> with the member data on success.
     */
    [[nodiscard]] GuildResult<GuildMemberData> GetMember(EterBase::EntityId id) const
    {
        if (!m_isEnabled)
        {
            return std::unexpected(GuildError::GuildNotEnabled);
        }

        if (auto it = m_members.find(id); it != m_members.end())
        {
            return it->second;
        }
        return std::unexpected(GuildError::MemberNotFound);
    }
    
    /**
     * @brief Retrieves a guild member by their name.
     * @param name The name of the member.
     * @return GuildResult<GuildMemberData> with the member data on success.
     */
    [[nodiscard]] GuildResult<GuildMemberData> GetMemberByName(std::string_view name) const
    {
        if (!m_isEnabled)
        {
            return std::unexpected(GuildError::GuildNotEnabled);
        }

        for (const auto& [id, member] : m_members)
        {
            if (member.name == name)
            {
                return member;
            }
        }
        return std::unexpected(GuildError::MemberNotFound);
    }

    /**
     * @brief Gets the total number of members in the guild.
     * @return The member count.
     */
    [[nodiscard]] uint32_t GetMemberCount() const noexcept
    {
        return static_cast<uint32_t>(m_members.size());
    }

    /**
     * @brief Registers or updates a guild skill.
     * @param skill The skill data to update.
     * @return GuildResult<void> indicating success.
     */
    GuildResult<void> UpsertSkill(const GuildSkillData& skill)
    {
        if (!m_isEnabled)
        {
            return std::unexpected(GuildError::GuildNotEnabled);
        }
        
        m_skills[skill.id] = skill;
        return {};
    }

    /**
     * @brief Retrieves data for a specific guild skill.
     * @param id The ID of the skill.
     * @return GuildResult<GuildSkillData> on success.
     */
    [[nodiscard]] GuildResult<GuildSkillData> GetSkill(EterBase::SkillId id) const
    {
        if (!m_isEnabled)
        {
            return std::unexpected(GuildError::GuildNotEnabled);
        }

        if (auto it = m_skills.find(id); it != m_skills.end())
        {
            return it->second;
        }
        return std::unexpected(GuildError::SkillNotFound);
    }

    /**
     * @brief Registers or updates a guild grade.
     * @param grade The grade data.
     */
    void RegisterGrade(const GuildGradeData& grade)
    {
        m_grades[grade.gradeNumber] = grade;
    }

    /**
     * @brief Retrieves data for a specific guild grade.
     * @param gradeNumber The numerical ID of the grade.
     * @return GuildResult<GuildGradeData> on success.
     */
    [[nodiscard]] GuildResult<GuildGradeData> GetGrade(uint8_t gradeNumber) const
    {
        if (auto it = m_grades.find(gradeNumber); it != m_grades.end())
        {
            return it->second;
        }
        return std::unexpected(GuildError::GradeNotFound);
    }

    /**
     * @brief Checks if a given ID matches the main player (guild master).
     * @param id The ID to check.
     * @return True if the ID is the master, false otherwise.
     */
    [[nodiscard]] bool IsMainPlayer(EterBase::EntityId id) const noexcept
    {
        return m_isEnabled && m_info.masterId == id;
    }
    
    /**
     * @brief Starts a guild war against the specified enemy guild.
     * @param enemyGuildId The ID of the enemy guild.
     */
    void StartGuildWar(EterBase::GuildId enemyGuildId)
    {
        // Add to active wars if not already present
        if (std::find(m_activeWars.begin(), m_activeWars.end(), enemyGuildId) == m_activeWars.end())
        {
            m_activeWars.push_back(enemyGuildId);
        }
    }

    /**
     * @brief Ends an active guild war against the specified enemy guild.
     * @param enemyGuildId The ID of the enemy guild.
     */
    void EndGuildWar(EterBase::GuildId enemyGuildId)
    {
        std::erase(m_activeWars, enemyGuildId);
    }

    /**
     * @brief Checks if the guild is currently in any active war.
     * @return True if at least one war is active, false otherwise.
     */
    [[nodiscard]] bool IsDoingGuildWar() const noexcept
    {
        return !m_activeWars.empty();
    }

    /**
     * @brief Clears all guild data.
     */
    void Clear()
    {
        m_isEnabled = false;
        m_info = GuildInfo{};
        m_members.clear();
        m_grades.clear();
        m_skills.clear();
        m_activeWars.clear();
    }

private:
    bool m_isEnabled;
    GuildInfo m_info;
    std::unordered_map<EterBase::EntityId, GuildMemberData> m_members;
    std::unordered_map<uint8_t, GuildGradeData> m_grades;
    std::unordered_map<EterBase::SkillId, GuildSkillData> m_skills;
    std::vector<EterBase::GuildId> m_activeWars;
};

} // namespace UserInterface::Domain
