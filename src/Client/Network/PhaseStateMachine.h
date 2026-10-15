#pragma once

#include "../../EterBase/Result.h"
#include <memory>
#include <unordered_map>

namespace Client::Network {

enum class Phase {
    Offline,
    Handshake,
    Login,
    Select,
    Loading,
    Game
};

class IPhase {
public:
    virtual ~IPhase() = default;

    virtual EterBase::VoidResult<> Enter() = 0;
    virtual EterBase::VoidResult<> Exit() = 0;
    virtual Phase GetPhase() const = 0;
};

class PhaseStateMachine {
public:
    PhaseStateMachine();
    ~PhaseStateMachine();

    void RegisterPhase(std::unique_ptr<IPhase> phase);
    
    EterBase::VoidResult<> ChangePhase(Phase newPhase);

    Phase GetCurrentPhase() const;

private:
    Phase m_currentPhase;
    IPhase* m_currentPhaseHandler;
    std::unordered_map<Phase, std::unique_ptr<IPhase>> m_phases;

    bool IsValidTransition(Phase from, Phase to) const;
};

} // namespace Client::Network
