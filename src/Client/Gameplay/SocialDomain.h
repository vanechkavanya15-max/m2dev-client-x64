#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace Client::Gameplay {

    // Party Member
    struct PartyMember {
        EterBase::EntityId vid;
        std::string name;
        uint8_t hpPercentage;
        float distance;
        bool isLeader;

        PartyMember(EterBase::EntityId _vid, const std::string& _name, bool _isLeader = false)
            : vid(_vid), name(_name), hpPercentage(100), distance(0.0f), isLeader(_isLeader) {}
    };

    // Party
    class Party {
    public:
        Party() = default;

        EterBase::VoidResult<> AddMember(const PartyMember& member);
        EterBase::VoidResult<> RemoveMember(EterBase::EntityId vid);
        EterBase::VoidResult<> SetLeader(EterBase::EntityId vid);
        EterBase::VoidResult<> UpdateMemberHP(EterBase::EntityId vid, uint8_t hpPercentage);
        EterBase::VoidResult<> UpdateMemberDistance(EterBase::EntityId vid, float distance);
        
        std::optional<PartyMember> GetMember(EterBase::EntityId vid) const;
        const std::vector<PartyMember>& GetMembers() const;
        std::optional<EterBase::EntityId> GetLeader() const;
        bool IsEmpty() const;

    private:
        std::vector<PartyMember> m_members;
    };

    // Guild Member
    struct GuildMember {
        EterBase::EntityId vid;
        std::string name;
        uint32_t permissions; // Bitmask for permissions

        GuildMember(EterBase::EntityId _vid, const std::string& _name, uint32_t _permissions = 0)
            : vid(_vid), name(_name), permissions(_permissions) {}
    };

    // Guild
    class Guild {
    public:
        Guild(EterBase::GuildId id, const std::string& name, uint8_t rank);

        EterBase::GuildId GetId() const;
        const std::string& GetName() const;
        uint8_t GetRank() const;
        const std::string& GetMark() const;
        void SetMark(const std::string& mark);

        EterBase::VoidResult<> AddMember(const GuildMember& member);
        EterBase::VoidResult<> RemoveMember(EterBase::EntityId vid);
        EterBase::VoidResult<> UpdateMemberPermissions(EterBase::EntityId vid, uint32_t permissions);
        std::optional<GuildMember> GetMember(EterBase::EntityId vid) const;
        const std::vector<GuildMember>& GetMembers() const;

        uint32_t GetBank() const;
        void SetBank(uint32_t bank);
        void SetExp(uint8_t level, uint32_t exp);
        std::pair<uint8_t, uint32_t> GetExp() const;

    private:
        EterBase::GuildId m_id;
        std::string m_name;
        uint8_t m_rank;
        std::string m_mark;
        uint32_t m_bank{0};
        uint8_t m_level{1};
        uint32_t m_exp{0};
        std::vector<GuildMember> m_members;
    };

    // Social Manager
    class SocialManager {
    public:
        SocialManager() = default;

        // Friend List
        EterBase::VoidResult<> AddFriend(const std::string& name);
        EterBase::VoidResult<> RemoveFriend(const std::string& name);
        bool IsFriend(const std::string& name) const;
        const std::vector<std::string>& GetFriends() const;

        // Ignore List
        EterBase::VoidResult<> AddIgnored(const std::string& name);
        EterBase::VoidResult<> RemoveIgnored(const std::string& name);
        bool IsIgnored(const std::string& name) const;
        const std::vector<std::string>& GetIgnored() const;

        // Party
        std::shared_ptr<Party> GetParty();
        void CreateParty();
        void LeaveParty();

        // Guild
        std::shared_ptr<Guild> GetGuild();
        void SetGuild(std::shared_ptr<Guild> guild);
        void LeaveGuild();

    private:
        std::vector<std::string> m_friends;
        std::vector<std::string> m_ignored;
        std::shared_ptr<Party> m_party;
        std::shared_ptr<Guild> m_guild;
    };

} // namespace Client::Gameplay
