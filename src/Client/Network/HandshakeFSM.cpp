#include "HandshakeFSM.h"
#include "../../EterBase/ModernLogger.h"

namespace Client::Network {

HandshakeFSM::HandshakeFSM(PhaseStateMachine* phaseMachine)
    : m_phaseMachine(phaseMachine),
      m_state(HandshakeState::Initial),
      m_serverTimeDelta(0) {
    if (!m_phaseMachine) {
        EterBase::ModernLogger::Error("HandshakeFSM initialized with null PhaseStateMachine.");
    }
}

EterBase::VoidResult<> HandshakeFSM::TransitionToHandshakeReceived(uint32_t clientTime, uint32_t serverTime, int32_t delta) {
    if (m_state != HandshakeState::Initial) {
        EterBase::ModernLogger::Error("Invalid FSM transition: Initial -> HandshakeReceived (Current state: {})", static_cast<int>(m_state));
        return EterBase::MakeError("Invalid state transition to HandshakeReceived");
    }

    // Calculate time synchronization delta
    // In legacy Ymir, client time - server time + delta
    // To properly support large numbers, calculate it carefully
    m_serverTimeDelta = static_cast<int32_t>(clientTime - serverTime) + delta;

    m_state = HandshakeState::HandshakeReceived;
    EterBase::ModernLogger::Info("HandshakeReceived: serverTimeDelta calculated as {}", m_serverTimeDelta);

    return {};
}

EterBase::VoidResult<> HandshakeFSM::TransitionToTimeSyncSent() {
    if (m_state != HandshakeState::HandshakeReceived) {
        EterBase::ModernLogger::Error("Invalid FSM transition: HandshakeReceived -> TimeSyncSent (Current state: {})", static_cast<int>(m_state));
        return EterBase::MakeError("Invalid state transition to TimeSyncSent");
    }

    m_state = HandshakeState::TimeSyncSent;
    EterBase::ModernLogger::Info("TimeSyncSent: Acknowledgment packet sent state tracked.");

    return {};
}

EterBase::VoidResult<> HandshakeFSM::TransitionToComplete() {
    if (m_state != HandshakeState::TimeSyncSent) {
        EterBase::ModernLogger::Error("Invalid FSM transition: TimeSyncSent -> Complete (Current state: {})", static_cast<int>(m_state));
        return EterBase::MakeError("Invalid state transition to Complete");
    }

    if (!m_phaseMachine) {
        EterBase::ModernLogger::Error("Complete transition failed: PhaseStateMachine is null.");
        return EterBase::MakeError("Null PhaseStateMachine");
    }

    auto phaseChangeResult = m_phaseMachine->ChangePhase(Phase::Login);
    if (!phaseChangeResult) {
        EterBase::ModernLogger::Error("Failed to switch phase to Login: {}", phaseChangeResult.error());
        return EterBase::MakeError("Phase change to Login failed");
    }

    m_state = HandshakeState::Complete;
    EterBase::ModernLogger::Info("Handshake Complete: Successfully transitioned to Login phase.");

    return {};
}

int32_t HandshakeFSM::GetServerTimeDelta() const noexcept {
    return m_serverTimeDelta;
}

HandshakeState HandshakeFSM::GetState() const noexcept {
    return m_state;
}

} // namespace Client::Network
