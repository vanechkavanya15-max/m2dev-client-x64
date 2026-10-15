#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <optional>
#include <functional>
#include <expected>
#include <algorithm>
#include <cstring>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace UserInterface::Domain {

/**
 * @brief Defines the possible roles a party member can have.
 */
enum class PartyRole : uint8_t {
    Normal = 0,
    Leader = 1,
    Attacker = 2,
    Defender = 3,
    Buffer = 4,
    SkillMaster = 5
};

/**
 * @brief Represents the data for a single member within a party.
 */
struct PartyMember {
    EterBase::EntityId id;      /**< Unique identifier of the member. */
    std::string name;           /**< Name of the member. */
    PartyRole role;             /**< Role of the member in the party. */
    uint8_t hpPercent;          /**< Health percentage of the member (0-100). */
};

#pragma pack(push, 1)
/**
 * @brief Raw network packet structure for updating a party member.
 * Strictly aligned for network serialization.
 */
struct PartyMemberUpdatePacket {
    uint8_t header;             /**< Packet header identifier. */
    uint32_t memberId;          /**< Target member's unique ID. */
    uint8_t role;               /**< The role enum value. */
    uint8_t hpPercent;          /**< Health percentage (0-100). */
    char name[32];              /**< Null-terminated or fixed-size string for member name. */
};
#pragma pack(pop)

/**
 * @brief Event triggered to notify the system that the party state has changed.
 *
 * Replaces direct Python UI calls to achieve decoupling.
 */
struct PartyUpdatedEvent : public Core::IEvent {
    EterBase::EntityId memberId; /**< ID of the member that triggered the update. */
    
    /**
     * @brief Constructs the event.
     * @param id The unique identifier of the member involved in the update.
     */
    explicit PartyUpdatedEvent(EterBase::EntityId id) : memberId(id) {}
};

/**
 * @brief Manages the domain logic and memory state for a party container.
 *
 * This class is strictly decoupled from the GUI layer and uses the EventBus
 * to publish state changes.
 */
class PartyContainerModel {
public:
    PartyContainerModel() = default;
    ~PartyContainerModel() = default;

    /**
     * @brief Adds a new member to the party.
     * @param member The member data to add.
     * @return std::expected<void, EterBase::EntityError> Success or EntityError::AlreadyExists.
     */
    std::expected<void, EterBase::EntityError> AddMember(const PartyMember& member) {
        auto it = std::ranges::find_if(m_members, [id = member.id](const PartyMember& m) { return m.id == id; });
        if (it != m_members.end()) {
            EterBase::ModernLogger::Error("PartyMember already exists: {}", member.id.value());
            return std::unexpected(EterBase::EntityError::AlreadyExists);
        }
        
        m_members.push_back(member);
        Core::EventBus::GetInstance().Publish(PartyUpdatedEvent{member.id});
        return {};
    }

    /**
     * @brief Removes a member from the party by ID.
     * @param memberId The ID of the member to remove.
     * @return std::expected<void, EterBase::EntityError> Success or EntityError::NotFound.
     */
    std::expected<void, EterBase::EntityError> RemoveMember(EterBase::EntityId memberId) {
        auto it = std::ranges::find_if(m_members, [memberId](const PartyMember& m) { return m.id == memberId; });
        if (it == m_members.end()) {
            EterBase::ModernLogger::Error("PartyMember not found for removal: {}", memberId.value());
            return std::unexpected(EterBase::EntityError::NotFound);
        }
        
        m_members.erase(it);
        Core::EventBus::GetInstance().Publish(PartyUpdatedEvent{memberId});
        return {};
    }

    /**
     * @brief Updates an existing member's properties.
     * @param memberId The ID of the member to update.
     * @param role The new role.
     * @param hpPercent The new HP percentage.
     * @return std::expected<void, EterBase::EntityError> Success or EntityError::NotFound.
     */
    std::expected<void, EterBase::EntityError> UpdateMember(EterBase::EntityId memberId, PartyRole role, uint8_t hpPercent) {
        auto it = std::ranges::find_if(m_members, [memberId](const PartyMember& m) { return m.id == memberId; });
        if (it == m_members.end()) {
            return std::unexpected(EterBase::EntityError::NotFound);
        }
        
        it->role = role;
        it->hpPercent = hpPercent;
        
        Core::EventBus::GetInstance().Publish(PartyUpdatedEvent{memberId});
        return {};
    }

    /**
     * @brief Clears the entire party.
     */
    void Clear() {
        m_members.clear();
        // Fire event with default ID to signal full refresh
        Core::EventBus::GetInstance().Publish(PartyUpdatedEvent{EterBase::EntityId{0}});
    }

    /**
     * @brief Retrieves a read-only span of all current party members.
     * @return std::span<const PartyMember> viewing the internal members.
     */
    std::span<const PartyMember> GetAllMembers() const {
        return m_members;
    }

    /**
     * @brief Processes a network update payload containing party member updates.
     * @param buffer The raw binary payload.
     * @return EterBase::PacketResult<void> Success or PacketError.
     */
    EterBase::PacketResult<void> ProcessNetworkUpdate(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(PartyMemberUpdatePacket)) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }

        size_t offset = 0;
        while (offset + sizeof(PartyMemberUpdatePacket) <= buffer.size()) {
            const auto* packet = reinterpret_cast<const PartyMemberUpdatePacket*>(buffer.data() + offset);
            
            EterBase::EntityId memberId{packet->memberId};
            
            auto it = std::ranges::find_if(m_members, [memberId](const PartyMember& m) { return m.id == memberId; });
            
            if (it != m_members.end()) {
                // Update existing
                it->role = static_cast<PartyRole>(packet->role);
                it->hpPercent = packet->hpPercent;
                Core::EventBus::GetInstance().Publish(PartyUpdatedEvent{memberId});
            } else {
                // Add new
                PartyMember newMember;
                newMember.id = memberId;
                newMember.role = static_cast<PartyRole>(packet->role);
                newMember.hpPercent = packet->hpPercent;
                
                size_t nameLength = strnlen(packet->name, sizeof(packet->name));
                newMember.name = std::string(packet->name, nameLength);
                
                m_members.push_back(newMember);
                Core::EventBus::GetInstance().Publish(PartyUpdatedEvent{memberId});
            }
            
            offset += sizeof(PartyMemberUpdatePacket);
        }

        return {};
    }

private:
    std::vector<PartyMember> m_members; /**< The collection of party members. */
};

} // namespace UserInterface::Domain
