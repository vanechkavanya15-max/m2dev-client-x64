#include "../src/Client/Network/HandshakeFSM.cpp"
#include "../src/Client/Network/PhaseStateMachine.cpp"
#include <iostream>
#include <cassert>

using namespace Client::Network;

class DummyPhase : public IPhase {
public:
    DummyPhase(Phase p) : m_phase(p) {}
    EterBase::VoidResult<> Enter() override { return {}; }
    EterBase::VoidResult<> Exit() override { return {}; }
    Phase GetPhase() const override { return m_phase; }
private:
    Phase m_phase;
};

void test_initial_state() {
    PhaseStateMachine psm;
    HandshakeFSM fsm(&psm);
    assert(fsm.GetState() == HandshakeState::Initial);
    assert(fsm.GetServerTimeDelta() == 0);
    std::cout << "test_initial_state passed\n";
}

void test_valid_transitions() {
    PhaseStateMachine psm;
    psm.RegisterPhase(std::make_unique<DummyPhase>(Phase::Offline));
    psm.RegisterPhase(std::make_unique<DummyPhase>(Phase::Handshake));
    psm.RegisterPhase(std::make_unique<DummyPhase>(Phase::Login));
    
    // Force PSM into Handshake phase first to allow valid transition to Login
    auto res1 = psm.ChangePhase(Phase::Handshake);
    assert(res1.has_value());

    HandshakeFSM fsm(&psm);

    // Initial -> HandshakeReceived
    uint32_t clientTime = 1000;
    uint32_t serverTime = 950;
    int32_t delta = 20;
    auto res2 = fsm.TransitionToHandshakeReceived(clientTime, serverTime, delta);
    assert(res2.has_value());
    assert(fsm.GetState() == HandshakeState::HandshakeReceived);
    // calc: 1000 - 950 + 20 = 70
    assert(fsm.GetServerTimeDelta() == 70);

    // HandshakeReceived -> TimeSyncSent
    auto res3 = fsm.TransitionToTimeSyncSent();
    assert(res3.has_value());
    assert(fsm.GetState() == HandshakeState::TimeSyncSent);

    // TimeSyncSent -> Complete
    auto res4 = fsm.TransitionToComplete();
    assert(res4.has_value());
    assert(fsm.GetState() == HandshakeState::Complete);
    assert(psm.GetCurrentPhase() == Phase::Login);

    std::cout << "test_valid_transitions passed\n";
}

void test_invalid_transitions() {
    PhaseStateMachine psm;
    HandshakeFSM fsm(&psm);

    // Invalid: Initial -> TimeSyncSent
    auto res1 = fsm.TransitionToTimeSyncSent();
    assert(!res1.has_value());
    assert(fsm.GetState() == HandshakeState::Initial);

    // Invalid: Initial -> Complete
    auto res2 = fsm.TransitionToComplete();
    assert(!res2.has_value());
    assert(fsm.GetState() == HandshakeState::Initial);

    std::cout << "test_invalid_transitions passed\n";
}

void test_time_delta_math() {
    PhaseStateMachine psm;
    HandshakeFSM fsm(&psm);

    // Test with wrap-around / larger delta values logic
    uint32_t clientTime = 500000;
    uint32_t serverTime = 600000; // server is ahead
    int32_t delta = -50; 
    // clientTime - serverTime is negative if we cast to signed properly or rely on uint wrap
    // 500000 - 600000 = -100000 
    // -100000 + (-50) = -100050

    auto res = fsm.TransitionToHandshakeReceived(clientTime, serverTime, delta);
    assert(res.has_value());
    assert(fsm.GetServerTimeDelta() == -100050);

    std::cout << "test_time_delta_math passed\n";
}

int main() {
    std::cout << "Running HandshakeFSM tests...\n";
    test_initial_state();
    test_valid_transitions();
    test_invalid_transitions();
    test_time_delta_math();
    std::cout << "All HandshakeFSM tests passed successfully.\n";
    return 0;
}
