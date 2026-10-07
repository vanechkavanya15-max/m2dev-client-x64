#include "../src/Client/Network/PhaseStateMachine.h"
#include "../src/EterBase/LogModern.h"
#include "../src/EterBase/Result.h"
#include <iostream>
#include <cassert>
#include <memory>
#include <string_view>

using namespace Client::Network;

class MockPhase : public IPhase {
public:
    MockPhase(Phase p) : m_phase(p) {}

    EterBase::VoidResult<> Enter() override {
        m_entered = true;
        EterBase::ModernLogger::Info("MockPhase {} Entered", static_cast<int>(m_phase));
        return {};
    }

    EterBase::VoidResult<> Exit() override {
        m_exited = true;
        EterBase::ModernLogger::Info("MockPhase {} Exited", static_cast<int>(m_phase));
        return {};
    }

    Phase GetPhase() const override { return m_phase; }

    bool m_entered = false;
    bool m_exited = false;
private:
    Phase m_phase;
};

void RunTest(std::string_view name, bool condition) {
    if (condition) {
        EterBase::ModernLogger::Info("TEST PASSED: {}", name);
    } else {
        EterBase::ModernLogger::Error("TEST FAILED: {}", name);
        std::exit(1);
    }
}

void TestInitialState() {
    PhaseStateMachine fsm;
    RunTest("Initial state is Offline", fsm.GetCurrentPhase() == Phase::Offline);
}

void TestValidTransitions() {
    PhaseStateMachine fsm;

    auto pOffline = std::make_unique<MockPhase>(Phase::Offline);
    auto pHandshake = std::make_unique<MockPhase>(Phase::Handshake);
    auto pLogin = std::make_unique<MockPhase>(Phase::Login);
    auto pSelect = std::make_unique<MockPhase>(Phase::Select);
    auto pLoading = std::make_unique<MockPhase>(Phase::Loading);
    auto pGame = std::make_unique<MockPhase>(Phase::Game);

    MockPhase* ptrOffline = pOffline.get();
    MockPhase* ptrHandshake = pHandshake.get();
    MockPhase* ptrLogin = pLogin.get();
    MockPhase* ptrSelect = pSelect.get();
    MockPhase* ptrLoading = pLoading.get();
    MockPhase* ptrGame = pGame.get();

    fsm.RegisterPhase(std::move(pOffline));
    fsm.RegisterPhase(std::move(pHandshake));
    fsm.RegisterPhase(std::move(pLogin));
    fsm.RegisterPhase(std::move(pSelect));
    fsm.RegisterPhase(std::move(pLoading));
    fsm.RegisterPhase(std::move(pGame));

    RunTest("Offline phase entered upon registration if current phase is Offline", ptrOffline->m_entered);
    
    auto res1 = fsm.ChangePhase(Phase::Handshake);
    RunTest("Offline -> Handshake transition successful", res1.has_value());
    RunTest("Offline exited", ptrOffline->m_exited);
    RunTest("Handshake entered", ptrHandshake->m_entered);

    auto res2 = fsm.ChangePhase(Phase::Login);
    RunTest("Handshake -> Login transition successful", res2.has_value());

    auto res3 = fsm.ChangePhase(Phase::Select);
    RunTest("Login -> Select transition successful", res3.has_value());

    auto res4 = fsm.ChangePhase(Phase::Loading);
    RunTest("Select -> Loading transition successful", res4.has_value());

    auto res5 = fsm.ChangePhase(Phase::Game);
    RunTest("Loading -> Game transition successful", res5.has_value());
    
    auto res6 = fsm.ChangePhase(Phase::Offline);
    RunTest("Game -> Offline transition successful", res6.has_value());
    RunTest("Game exited", ptrGame->m_exited);
}

void TestInvalidTransitions() {
    PhaseStateMachine fsm;

    fsm.RegisterPhase(std::make_unique<MockPhase>(Phase::Offline));
    fsm.RegisterPhase(std::make_unique<MockPhase>(Phase::Handshake));
    fsm.RegisterPhase(std::make_unique<MockPhase>(Phase::Login));
    fsm.RegisterPhase(std::make_unique<MockPhase>(Phase::Game));

    auto res1 = fsm.ChangePhase(Phase::Login);
    RunTest("Offline -> Login should fail (must go through Handshake)", !res1.has_value());

    fsm.ChangePhase(Phase::Handshake);
    fsm.ChangePhase(Phase::Login);

    auto res2 = fsm.ChangePhase(Phase::Game);
    RunTest("Login -> Game should fail (must go through Select and Loading)", !res2.has_value());
    RunTest("State remained Login after invalid transition", fsm.GetCurrentPhase() == Phase::Login);
}

void TestMissingPhaseHandler() {
    PhaseStateMachine fsm;
    fsm.RegisterPhase(std::make_unique<MockPhase>(Phase::Offline));
    // Do not register Handshake
    
    auto res = fsm.ChangePhase(Phase::Handshake);
    RunTest("Transition to unregistered phase fails", !res.has_value());
    RunTest("Error is Phase handler not registered", res.error() == "Phase handler not registered.");
}

int main() {
    EterBase::ModernLogger::Info("Starting PhaseStateMachine tests...");
    
    TestInitialState();
    TestValidTransitions();
    TestInvalidTransitions();
    TestMissingPhaseHandler();

    EterBase::ModernLogger::Info("All tests completed successfully!");
    return 0;
}
