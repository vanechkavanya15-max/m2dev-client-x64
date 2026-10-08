#include "StdAfx.h"

#include "SocialDomain.h"
#include <algorithm>

namespace Client::Gameplay {

    // ============================================================================
    // Party Implementation
    // ============================================================================

    EterBase::VoidResult<> Party::AddMember(const PartyMember& member) {
        if (std::ranges::find_if(m_members, [&](const auto& m) { return m.vid == member.vid; }) != m_members.end()) {
            return EterBase::MakeError("Member already exists in party.");
        }
        
        m_members.push_back(member);
        
        // If it's the first member and they are set as leader, fine.
        // If not the first and they are leader, we need to unset previous leader.
        if (member.isLeader) {
            for (auto& m : m_members) {
                if (m.vid != member.vid) {
                    m.isLeader = false;
                }
            }
        } else if (m_members.size() == 1) {
            // First member added is always leader
            m_members.front().isLeader = true;
        }

        return {};
    }

    EterBase::VoidResult<> Party::RemoveMember(EterBase::EntityId vid) {
        auto it = std::ranges::find_if(m_members, [&](const auto& m) { return m.vid == vid; });
        if (it == m_members.end()) {
            return EterBase::MakeError("Member not found in party.");
        }

        bool wasLeader = it->isLeader;
        m_members.erase(it);

        if (wasLeader && !m_members.empty()) {
            m_members.front().isLeader = true; // Reassign leader to first available
        }

        return {};
    }

    EterBase::VoidResult<> Party::SetLeader(EterBase::EntityId vid) {
        auto it = std::ranges::find_if(m_members, [&](const auto& m) { return m.vid == vid; });
        if (it == m_members.end()) {
            return EterBase::MakeError("Member not found in party.");
        }

        for (auto& m : m_members) {
            m.isLeader = (m.vid == vid);
        }

        return {};
    }

    EterBase::VoidResult<> Party::UpdateMemberHP(EterBase::EntityId vid, uint8_t hpPercentage) {
        auto it = std::ranges::find_if(m_members, [&](const auto& m) { return m.vid == vid; });
        if (it == m_members.end()) {
            return EterBase::MakeError("Member not found in party.");
        }
        it->hpPercentage = hpPercentage;
        return {};
    }

    EterBase::VoidResult<> Party::UpdateMemberDistance(EterBase::EntityId vid, float distance) {
        auto it = std::ranges::find_if(m_members, [&](const auto& m) { return m.vid == vid; });
        if (it == m_members.end()) {
            return EterBase::MakeError("Member not found in party.");
        }
        it->distance = distance;
        return {};
    }

    std::optional<PartyMember> Party::GetMember(EterBase::EntityId vid) const {
        auto it = std::ranges::find_if(m_members, [&](const auto& m) { return m.vid == vid; });
        if (it != m_members.end()) {
            return *it;
        }
        return std::nullopt;
    }

    const std::vector<PartyMember>& Party::GetMembers() const {
        return m_members;
    }

    std::optional<EterBase::EntityId> Party::GetLeader() const {
        auto it = std::ranges::find_if(m_members, [](const auto& m) { return m.isLeader; });
        if (it != m_members.end()) {
            return it->vid;
        }
        return std::nullopt;
    }

    bool Party::IsEmpty() const {
        return m_members.empty();
    }


    // ============================================================================
    // Guild Implementation
    // ============================================================================

    Guild::Guild(EterBase::GuildId id, const std::string& name, uint8_t rank)
        : m_id(id), m_name(name), m_rank(rank) {}

    EterBase::GuildId Guild::GetId() const {
        return m_id;
    }

    const std::string& Guild::GetName() const {
        return m_name;
    }

    uint8_t Guild::GetRank() const {
        return m_rank;
    }

    const std::string& Guild::GetMark() const {
        return m_mark;
    }

    void Guild::SetMark(const std::string& mark) {
        m_mark = mark;
    }

    uint32_t Guild::GetBank() const {
        return m_bank;
    }

    void Guild::SetBank(uint32_t bank) {
        m_bank = bank;
    }

    void Guild::SetExp(uint8_t level, uint32_t exp) {
        m_level = level;
        m_exp = exp;
    }

    std::pair<uint8_t, uint32_t> Guild::GetExp() const {
        return {m_level, m_exp};
    }

    EterBase::VoidResult<> Guild::AddMember(const GuildMember& member) {
        if (std::ranges::find_if(m_members, [&](const auto& m) { return m.vid == member.vid; }) != m_members.end()) {
            return EterBase::MakeError("Member already exists in guild.");
        }
        m_members.push_back(member);
        return {};
    }

    EterBase::VoidResult<> Guild::RemoveMember(EterBase::EntityId vid) {
        auto it = std::ranges::find_if(m_members, [&](const auto& m) { return m.vid == vid; });
        if (it == m_members.end()) {
            return EterBase::MakeError("Member not found in guild.");
        }
        m_members.erase(it);
        return {};
    }

    EterBase::VoidResult<> Guild::UpdateMemberPermissions(EterBase::EntityId vid, uint32_t permissions) {
        auto it = std::ranges::find_if(m_members, [&](const auto& m) { return m.vid == vid; });
        if (it == m_members.end()) {
            return EterBase::MakeError("Member not found in guild.");
        }
        it->permissions = permissions;
        return {};
    }

    std::optional<GuildMember> Guild::GetMember(EterBase::EntityId vid) const {
        auto it = std::ranges::find_if(m_members, [&](const auto& m) { return m.vid == vid; });
        if (it != m_members.end()) {
            return *it;
        }
        return std::nullopt;
    }

    const std::vector<GuildMember>& Guild::GetMembers() const {
        return m_members;
    }


    // ============================================================================
    // Social Manager Implementation
    // ============================================================================

    EterBase::VoidResult<> SocialManager::AddFriend(const std::string& name) {
        if (std::ranges::find(m_friends, name) != m_friends.end()) {
            return EterBase::MakeError("Friend already exists in friend list.");
        }
        if (std::ranges::find(m_ignored, name) != m_ignored.end()) {
            return EterBase::MakeError("Cannot add friend who is ignored.");
        }
        m_friends.push_back(name);
        return {};
    }

    EterBase::VoidResult<> SocialManager::RemoveFriend(const std::string& name) {
        auto it = std::ranges::find(m_friends, name);
        if (it == m_friends.end()) {
            return EterBase::MakeError("Friend not found in friend list.");
        }
        m_friends.erase(it);
        return {};
    }

    bool SocialManager::IsFriend(const std::string& name) const {
        return std::ranges::find(m_friends, name) != m_friends.end();
    }

    const std::vector<std::string>& SocialManager::GetFriends() const {
        return m_friends;
    }

    EterBase::VoidResult<> SocialManager::AddIgnored(const std::string& name) {
        if (std::ranges::find(m_ignored, name) != m_ignored.end()) {
            return EterBase::MakeError("Player already exists in ignore list.");
        }
        if (std::ranges::find(m_friends, name) != m_friends.end()) {
            return EterBase::MakeError("Cannot ignore a friend.");
        }
        m_ignored.push_back(name);
        return {};
    }

    EterBase::VoidResult<> SocialManager::RemoveIgnored(const std::string& name) {
        auto it = std::ranges::find(m_ignored, name);
        if (it == m_ignored.end()) {
            return EterBase::MakeError("Player not found in ignore list.");
        }
        m_ignored.erase(it);
        return {};
    }

    bool SocialManager::IsIgnored(const std::string& name) const {
        return std::ranges::find(m_ignored, name) != m_ignored.end();
    }

    const std::vector<std::string>& SocialManager::GetIgnored() const {
        return m_ignored;
    }

    std::shared_ptr<Party> SocialManager::GetParty() {
        return m_party;
    }

    void SocialManager::CreateParty() {
        if (!m_party) {
            m_party = std::make_shared<Party>();
        }
    }

    void SocialManager::LeaveParty() {
        m_party.reset();
    }

    std::shared_ptr<Guild> SocialManager::GetGuild() {
        return m_guild;
    }

    void SocialManager::SetGuild(std::shared_ptr<Guild> guild) {
        m_guild = guild;
    }

    void SocialManager::LeaveGuild() {
        m_guild.reset();
    }

} // namespace Client::Gameplay
