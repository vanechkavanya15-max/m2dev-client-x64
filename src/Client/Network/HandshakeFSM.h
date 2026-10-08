#pragma once

#include <cstdint>
#include "../../EterBase/Result.h"
#include "PhaseStateMachine.h"

namespace Client::Network {

/**
 * @brief States for the Handshake FSM.
 */
enum class HandshakeState : uint8_t {
    Initial,
    HandshakeReceived,
    TimeSyncSent,
    Complete
};

/**
 * @brief Represents the Handshake Finite State Machine.
 *
 * Implements modern C++23 zero-conflict architecture to safely orchestrate
 * the Ymir 13B handshake and phase transition logic.
 */
class HandshakeFSM {
public:
    /**
     * @brief Constructor requiring a valid phase state machine to transition.
     * @param phaseMachine Pointer to the active PhaseStateMachine.
     */
    explicit HandshakeFSM(PhaseStateMachine* phaseMachine);

    ~HandshakeFSM() = default;

    // Delete copy/move to enforce single instance zero-conflict principle
    HandshakeFSM(const HandshakeFSM&) = delete;
    HandshakeFSM& operator=(const HandshakeFSM&) = delete;
    HandshakeFSM(HandshakeFSM&&) = delete;
    HandshakeFSM& operator=(HandshakeFSM&&) = delete;

    /**
     * @brief Transitions the FSM when the initial handshake packet is received.
     * 
     * @param clientTime Current local client time in ms.
     * @param serverTime Server time extracted from packet in ms.
     * @param delta Server delta time from packet.
     * @return VoidResult<> success or error if transition is invalid.
     */
    EterBase::VoidResult<> TransitionToHandshakeReceived(uint32_t clientTime, uint32_t serverTime, int32_t delta);

    /**
     * @brief Transitions the FSM after sending the time sync acknowledgment.
     * @return VoidResult<> success or error if transition is invalid.
     */
    EterBase::VoidResult<> TransitionToTimeSyncSent();

    /**
     * @brief Completes the handshake and switches the phase to Login.
     * @return VoidResult<> success or error if transition is invalid.
     */
    EterBase::VoidResult<> TransitionToComplete();

    /**
     * @brief Returns the calculated server time delta for synchronization.
     * @return Delta in milliseconds. Returns 0 if not calculated yet.
     */
    [[nodiscard]] int32_t GetServerTimeDelta() const noexcept;

    /**
     * @brief Returns the current state of the FSM.
     * @return HandshakeState
     */
    [[nodiscard]] HandshakeState GetState() const noexcept;

private:
    PhaseStateMachine* m_phaseMachine;
    HandshakeState m_state;
    int32_t m_serverTimeDelta;
};

} // namespace Client::Network
