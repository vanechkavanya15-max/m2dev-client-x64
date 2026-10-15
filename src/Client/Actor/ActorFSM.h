#pragma once

#include <expected>
#include <string_view>
#include <format>
#include <cstdint>

namespace Client::Actor {

enum class ActorState : uint8_t {
    Idle,
    Walk,
    Run,
    Attack,
    Skill,
    Stun,
    Dead
};

enum class ActorEvent : uint8_t {
    OnMove,
    OnRun,
    OnStop,
    OnAttack,
    OnSkill,
    OnStun,
    OnDeath,
    OnRevive
};

enum class ActorFSMError : uint8_t {
    InvalidTransition,
    DeadStateIsTerminal,
    AlreadyInState
};

constexpr std::string_view to_string(ActorFSMError error) noexcept {
    switch (error) {
        case ActorFSMError::InvalidTransition: return "InvalidTransition";
        case ActorFSMError::DeadStateIsTerminal: return "DeadStateIsTerminal";
        case ActorFSMError::AlreadyInState: return "AlreadyInState";
        default: return "UnknownError";
    }
}

constexpr std::string_view to_string(ActorState state) noexcept {
    switch (state) {
        case ActorState::Idle: return "Idle";
        case ActorState::Walk: return "Walk";
        case ActorState::Run: return "Run";
        case ActorState::Attack: return "Attack";
        case ActorState::Skill: return "Skill";
        case ActorState::Stun: return "Stun";
        case ActorState::Dead: return "Dead";
        default: return "Unknown";
    }
}

template <typename T>
using Result = std::expected<T, ActorFSMError>;

class ActorFSM {
public:
    constexpr ActorFSM() noexcept : m_currentState(ActorState::Idle) {}
    constexpr explicit ActorFSM(ActorState initialState) noexcept : m_currentState(initialState) {}

    [[nodiscard]] constexpr ActorState GetState() const noexcept {
        return m_currentState;
    }

    constexpr Result<ActorState> HandleEvent(ActorEvent event) noexcept {
        if (m_currentState == ActorState::Dead && event != ActorEvent::OnRevive) {
            return std::unexpected(ActorFSMError::DeadStateIsTerminal);
        }

        switch (event) {
            case ActorEvent::OnMove:
                if (m_currentState == ActorState::Stun) {
                    return std::unexpected(ActorFSMError::InvalidTransition);
                }
                if (m_currentState == ActorState::Walk) {
                    return std::unexpected(ActorFSMError::AlreadyInState);
                }
                return TransitionTo(ActorState::Walk);

            case ActorEvent::OnRun:
                if (m_currentState == ActorState::Stun) {
                    return std::unexpected(ActorFSMError::InvalidTransition);
                }
                if (m_currentState == ActorState::Run) {
                    return std::unexpected(ActorFSMError::AlreadyInState);
                }
                return TransitionTo(ActorState::Run);

            case ActorEvent::OnStop:
                if (m_currentState == ActorState::Stun) {
                    return std::unexpected(ActorFSMError::InvalidTransition);
                }
                if (m_currentState == ActorState::Idle) {
                    return std::unexpected(ActorFSMError::AlreadyInState);
                }
                return TransitionTo(ActorState::Idle);

            case ActorEvent::OnAttack:
                if (m_currentState == ActorState::Stun) {
                    return std::unexpected(ActorFSMError::InvalidTransition);
                }
                if (m_currentState == ActorState::Attack) {
                    return std::unexpected(ActorFSMError::AlreadyInState);
                }
                return TransitionTo(ActorState::Attack);

            case ActorEvent::OnSkill:
                if (m_currentState == ActorState::Stun) {
                    return std::unexpected(ActorFSMError::InvalidTransition);
                }
                if (m_currentState == ActorState::Skill) {
                    return std::unexpected(ActorFSMError::AlreadyInState);
                }
                return TransitionTo(ActorState::Skill);

            case ActorEvent::OnStun:
                if (m_currentState == ActorState::Stun) {
                    return std::unexpected(ActorFSMError::AlreadyInState);
                }
                return TransitionTo(ActorState::Stun);

            case ActorEvent::OnDeath:
                return TransitionTo(ActorState::Dead);
            
            case ActorEvent::OnRevive:
                if (m_currentState != ActorState::Dead) {
                    return std::unexpected(ActorFSMError::InvalidTransition);
                }
                return TransitionTo(ActorState::Idle);
        }
        
        return std::unexpected(ActorFSMError::InvalidTransition);
    }

private:
    constexpr Result<ActorState> TransitionTo(ActorState newState) noexcept {
        m_currentState = newState;
        return m_currentState;
    }

    ActorState m_currentState;
};

} // namespace Client::Actor

template <>
struct std::formatter<Client::Actor::ActorFSMError> : std::formatter<std::string_view> {
    auto format(Client::Actor::ActorFSMError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Actor::to_string(err), ctx);
    }
};

template <>
struct std::formatter<Client::Actor::ActorState> : std::formatter<std::string_view> {
    auto format(Client::Actor::ActorState state, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Actor::to_string(state), ctx);
    }
};
