#pragma once

#include <cstdint>
#include <string>

namespace Client::Events {

/**
 * @brief Zdarzenie domenowe oznaczajace dolaczenie nowego czlonka do grupy.
 */
struct PartyMemberJoinedEvent {
    uint32_t memberVid{0};
    std::string memberName;

    constexpr PartyMemberJoinedEvent() = default;
    
    // std::string isn't constexpr in all compilers up to C++20 fully in this context, 
    // but C++20 allows constexpr std::string.
    constexpr PartyMemberJoinedEvent(uint32_t vid, std::string name)
        : memberVid(vid), memberName(std::move(name)) {}

    constexpr bool operator==(const PartyMemberJoinedEvent& other) const {
        return memberVid == other.memberVid && memberName == other.memberName;
    }
};

/**
 * @brief Zdarzenie domenowe zmiany lidera grupy.
 */
struct PartyLeaderChangedEvent {
    uint32_t oldLeaderVid{0};
    uint32_t newLeaderVid{0};

    constexpr PartyLeaderChangedEvent() noexcept = default;
    constexpr PartyLeaderChangedEvent(uint32_t oldLeaderVid, uint32_t newLeaderVid) noexcept
        : oldLeaderVid(oldLeaderVid), newLeaderVid(newLeaderVid) {}

    constexpr bool operator==(const PartyLeaderChangedEvent& other) const noexcept = default;
};

/**
 * @brief Zdarzenie domenowe rozpoczecia wojny gildii.
 */
struct GuildWarStartedEvent {
    uint32_t challengerGuildId{0};
    uint32_t opponentGuildId{0};

    constexpr GuildWarStartedEvent() noexcept = default;
    constexpr GuildWarStartedEvent(uint32_t challengerGuildId, uint32_t opponentGuildId) noexcept
        : challengerGuildId(challengerGuildId), opponentGuildId(opponentGuildId) {}

    constexpr bool operator==(const GuildWarStartedEvent& other) const noexcept = default;
};

} // namespace Client::Events
