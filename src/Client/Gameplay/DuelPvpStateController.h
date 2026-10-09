#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace Client::Gameplay {

enum class PvpMode : uint8_t {
    Peace,
    Revenge,
    Free,
    Guild
};

enum class DuelState : uint8_t {
    None,
    Challenged,
    Active,
    Finished
};

enum class PvpError : uint8_t {
    InvalidStateTransition,
    AlreadyInDuel,
    NotInDuel,
    CannotChangeModeInDuel
};

[[nodiscard]] constexpr std::string_view ToString(PvpError error) noexcept {
    switch (error) {
        case PvpError::InvalidStateTransition: return "Invalid state transition";
        case PvpError::AlreadyInDuel: return "Already in duel";
        case PvpError::NotInDuel: return "Not in duel";
        case PvpError::CannotChangeModeInDuel: return "Cannot change PVP mode while in duel";
    }
    return "Unknown error";
}

class DuelPvpStateController {
public:
    constexpr DuelPvpStateController() noexcept = default;

    [[nodiscard]] constexpr PvpMode GetPvpMode() const noexcept {
        return m_pvpMode;
    }

    [[nodiscard]] constexpr DuelState GetDuelState() const noexcept {
        return m_duelState;
    }

    constexpr std::expected<void, PvpError> SetPvpMode(PvpMode newMode) noexcept {
        if (m_duelState == DuelState::Active) {
            return std::unexpected(PvpError::CannotChangeModeInDuel);
        }
        m_pvpMode = newMode;
        return {};
    }

    constexpr std::expected<void, PvpError> TransitionDuelState(DuelState newState) noexcept {
        switch (newState) {
            case DuelState::Challenged:
                if (m_duelState != DuelState::None) {
                    return std::unexpected(PvpError::AlreadyInDuel);
                }
                break;
            case DuelState::Active:
                if (m_duelState != DuelState::Challenged) {
                    return std::unexpected(PvpError::InvalidStateTransition);
                }
                break;
            case DuelState::Finished:
                if (m_duelState != DuelState::Active && m_duelState != DuelState::Challenged) {
                    return std::unexpected(PvpError::NotInDuel);
                }
                break;
            case DuelState::None:
                if (m_duelState != DuelState::Finished && m_duelState != DuelState::Challenged) {
                     return std::unexpected(PvpError::InvalidStateTransition);
                }
                break;
        }

        m_duelState = newState;
        return {};
    }

private:
    PvpMode m_pvpMode{PvpMode::Peace};
    DuelState m_duelState{DuelState::None};
};

} // namespace Client::Gameplay
