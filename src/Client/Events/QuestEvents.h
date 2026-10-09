#pragma once

#include <cstdint>

namespace Client::Events {

/**
 * @brief Zdarzenie domenowe otwarcia tablicy zadan (Quest Board).
 */
struct QuestBoardOpenedEvent {
    uint32_t npcVid{0};

    constexpr QuestBoardOpenedEvent() = default;
    constexpr explicit QuestBoardOpenedEvent(uint32_t npcVid) noexcept
        : npcVid(npcVid) {}

    constexpr bool operator==(const QuestBoardOpenedEvent& other) const = default;
};

/**
 * @brief Zdarzenie domenowe wyboru odpowiedzi w dialogu z NPC.
 */
struct QuestDialogAnsweredEvent {
    uint32_t npcVid{0};
    uint8_t answerIndex{0};

    constexpr QuestDialogAnsweredEvent() = default;
    constexpr QuestDialogAnsweredEvent(uint32_t npcVid, uint8_t answerIndex) noexcept
        : npcVid(npcVid), answerIndex(answerIndex) {}

    constexpr bool operator==(const QuestDialogAnsweredEvent& other) const = default;
};

} // namespace Client::Events
