#include "PhaseStateMachine.h"
#include "../../EterBase/LogModern.h"

namespace Client::Network {

PhaseStateMachine::PhaseStateMachine() 
    : m_currentPhase(Phase::Offline)
    , m_currentPhaseHandler(nullptr)
{
}

PhaseStateMachine::~PhaseStateMachine() {
    if (m_currentPhaseHandler) {
        auto exitResult = m_currentPhaseHandler->Exit();
        if (!exitResult.has_value()) {
            EterBase::ModernLogger::Error("Failed to exit current phase during destruction: {}", exitResult.error());
        }
    }
}

void PhaseStateMachine::RegisterPhase(std::unique_ptr<IPhase> phase) {
    if (!phase) {
        return;
    }
    
    Phase p = phase->GetPhase();
    m_phases[p] = std::move(phase);
    
    // Initialize the offline phase if it's registered
    if (p == Phase::Offline && m_currentPhase == Phase::Offline && !m_currentPhaseHandler) {
        m_currentPhaseHandler = m_phases[Phase::Offline].get();
        auto res = m_currentPhaseHandler->Enter();
        if (!res.has_value()) {
            EterBase::ModernLogger::Error("Failed to enter offline phase during registration: {}", res.error());
        }
    }
}

Phase PhaseStateMachine::GetCurrentPhase() const {
    return m_currentPhase;
}

bool PhaseStateMachine::IsValidTransition(Phase from, Phase to) const {
    // We can always go back to offline (e.g., error, disconnect, closed)
    if (to == Phase::Offline) {
        return true;
    }

    switch (from) {
        case Phase::Offline:
            return to == Phase::Handshake;
        case Phase::Handshake:
            return to == Phase::Login;
        case Phase::Login:
            return to == Phase::Select;
        case Phase::Select:
            return to == Phase::Loading || to == Phase::Login;
        case Phase::Loading:
            return to == Phase::Game || to == Phase::Select; // Sometimes back to select if loading fails/aborted
        case Phase::Game:
            return to == Phase::Select; // Back to character selection
        default:
            return false;
    }
}

EterBase::VoidResult<> PhaseStateMachine::ChangePhase(Phase newPhase) {
    if (m_currentPhase == newPhase) {
        return {}; // Already in this phase
    }

    if (!IsValidTransition(m_currentPhase, newPhase)) {
        EterBase::ModernLogger::Error("Invalid phase transition requested. From: {} To: {}", 
                                      static_cast<int>(m_currentPhase), 
                                      static_cast<int>(newPhase));
        return EterBase::MakeError("Invalid phase transition.");
    }

    auto it = m_phases.find(newPhase);
    if (it == m_phases.end()) {
        EterBase::ModernLogger::Error("Requested phase is not registered: {}", static_cast<int>(newPhase));
        return EterBase::MakeError("Phase handler not registered.");
    }

    IPhase* nextPhaseHandler = it->second.get();

    // Cleanup current phase
    if (m_currentPhaseHandler) {
        auto exitResult = m_currentPhaseHandler->Exit();
        if (!exitResult.has_value()) {
            EterBase::ModernLogger::Error("Failed to cleanly exit phase {}: {}", 
                                          static_cast<int>(m_currentPhase), 
                                          exitResult.error());
            // Zależnie od polityki, błąd wyjścia może powstrzymać wejście. Przyjmujemy, że musimy zwrócić błąd.
            return EterBase::MakeError("Failed to exit current phase.");
        }
    }

    // Enter new phase
    auto enterResult = nextPhaseHandler->Enter();
    if (!enterResult.has_value()) {
        EterBase::ModernLogger::Error("Failed to enter phase {}: {}", 
                                      static_cast<int>(newPhase), 
                                      enterResult.error());
        return EterBase::MakeError("Failed to enter new phase.");
    }

    m_currentPhase = newPhase;
    m_currentPhaseHandler = nextPhaseHandler;

    EterBase::ModernLogger::Info("Phase transition successful. New Phase: {}", static_cast<int>(newPhase));

    return {};
}

} // namespace Client::Network
