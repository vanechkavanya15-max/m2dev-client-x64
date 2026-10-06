#include "../../StdAfx.h"
/**
 * @file GuildMemberHandler.cpp
 * @brief Handler for receiving and saving guild member packets from the server.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <span>
#include <string_view>
#include <iostream>
#include <cstring>
#include "../../Packet.h"

namespace Network {
namespace Handlers {

/**
 * @brief Structure representing a single guild member.
 */
struct GuildMember {
    /** @brief The unique ID of the guild member. */
    uint32_t id{0};
    
    /** @brief The name of the guild member. */
    std::string name;
    
    /** @brief The rank (grade) of the guild member. */
    uint8_t rank{0};
    
    /** @brief The level of the guild member. */
    uint8_t level{0};
};

/**
 * @brief Handles operations related to guild members, saving their states securely in C++ memory.
 */
class GuildMemberHandler {
public:
    GuildMemberHandler() = default;
    ~GuildMemberHandler() = default;

    /**
     * @brief Parses a guild member packet buffer and stores the member data.
     * @param buffer A span over the incoming byte buffer containing the sub-member packet.
     * @return True if parsing succeeded, false if buffer is too small.
     */
    bool HandleGuildMemberPacket(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCGuildSubMember)) {
            return false;
        }

        const TPacketGCGuildSubMember* packet = reinterpret_cast<const TPacketGCGuildSubMember*>(buffer.data());
        
        GuildMember newMember;
        newMember.id = packet->pid;
        newMember.rank = packet->byGrade;
        newMember.level = packet->byLevel;

        size_t currentOffset = sizeof(TPacketGCGuildSubMember);

        // If NameFlag is set, the server appends the name to the packet payload.
        if (packet->byNameFlag) {
            constexpr size_t nameBufferSize = CHARACTER_NAME_MAX_LEN + 1;
            if (buffer.size() < currentOffset + nameBufferSize) {
                return false;
            }

            const char* rawName = reinterpret_cast<const char*>(buffer.data() + currentOffset);
            newMember.name = std::string(rawName, strnlen(rawName, CHARACTER_NAME_MAX_LEN));
            currentOffset += nameBufferSize;
        }

        // Check if member already exists and update
        for (auto& member : members_) {
            if (member.id == newMember.id) {
                member.rank = newMember.rank;
                member.level = newMember.level;
                if (!newMember.name.empty()) {
                    member.name = newMember.name;
                }
                return true;
            }
        }

        members_.push_back(std::move(newMember));
        return true;
    }

    /**
     * @brief Gets all saved guild members.
     * @return A constant reference to the internal member vector.
     */
    const std::vector<GuildMember>& GetMembers() const {
        return members_;
    }

    /**
     * @brief Clears all currently saved guild members.
     */
    void Clear() {
        members_.clear();
    }

private:
    /** @brief List of stored guild members. */
    std::vector<GuildMember> members_;
};

} // namespace Handlers
} // namespace Network
