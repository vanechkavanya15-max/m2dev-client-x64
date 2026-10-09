#pragma once

#include <cstdint>
#include <span>

#include "../../../EterBase/Result.h"
#include "../ModernPacketDispatcher.h"
#include "../PhaseStateMachine.h"

#ifndef TEST_MODE_DISABLE_STDAFX
#include "../Protocol/Protocol.h"
#endif

namespace Client::Network
{
    /**
     * @brief Modul Handlera Ppakietow zmiany fazy (PhaseSwitch).
     * 
     * Odbiera pakiet TPacketGCPhase i aktualizuje stan w PhaseStateMachine.
     * ZERO-CONFLICT: Niezalezne rozszerzenie z obsluga bledow.
     */
    class PhaseSwitchPacketHandler final : public IPacketHandler
    {
    public:
        explicit PhaseSwitchPacketHandler(PhaseStateMachine* stateMachine) noexcept;
        ~PhaseSwitchPacketHandler() override = default;

        // Disallow copy & move
        PhaseSwitchPacketHandler(const PhaseSwitchPacketHandler&) = delete;
        PhaseSwitchPacketHandler& operator=(const PhaseSwitchPacketHandler&) = delete;
        PhaseSwitchPacketHandler(PhaseSwitchPacketHandler&&) = delete;
        PhaseSwitchPacketHandler& operator=(PhaseSwitchPacketHandler&&) = delete;

        [[nodiscard]] EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) override;
        [[nodiscard]] uint16_t GetExpectedSize() const override;
        [[nodiscard]] bool IsDynamicSize() const override;

    private:
        PhaseStateMachine* m_stateMachine;
    };
}
