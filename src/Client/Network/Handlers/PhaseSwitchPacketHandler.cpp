#include "PhaseSwitchPacketHandler.h"

#ifndef TEST_MODE_DISABLE_STDAFX
#include "EterLib/ControlPackets.h"
#include "EterBase/LogModern.h"
#else

#pragma pack(push, 1)
struct TPacketGCPhase
{
    uint16_t header;
    uint16_t length;
    uint8_t  phase;
};
#pragma pack(pop)

#include "EterBase/LogModern.h"
#endif

namespace Client::Network
{
    PhaseSwitchPacketHandler::PhaseSwitchPacketHandler(PhaseStateMachine* stateMachine) noexcept
        : m_stateMachine(stateMachine)
    {
    }

    EterBase::PacketResult<void> PhaseSwitchPacketHandler::Handle(std::span<const uint8_t> payload)
    {
        if (!m_stateMachine)
        {
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload); // or create a specific error
        }

        if (payload.size() < sizeof(TPacketGCPhase))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCPhase*>(payload.data());
        
        Phase nextPhase = Phase::Offline;

        // Mapowanie wartosci numerycznych na silnie typowane wartosci fazy z PhaseStateMachine
        switch (packet->phase)
        {
            case 0: // PHASE_CLOSE / Phase::Close w ProtocolOpcodes.h -> Offline
                nextPhase = Phase::Offline;
                break;
            case 1: // PHASE_HANDSHAKE
                nextPhase = Phase::Handshake;
                break;
            case 2: // PHASE_LOGIN
                nextPhase = Phase::Login;
                break;
            case 3: // PHASE_SELECT
                nextPhase = Phase::Select;
                break;
            case 4: // PHASE_LOADING
                nextPhase = Phase::Loading;
                break;
            case 5: // PHASE_GAME
                nextPhase = Phase::Game;
                break;
            default:
                // Nieznana faza. Uznajemy za blad parsowania badz niepoprawna wartosc
                EterBase::ModernLogger::Warning("PhaseSwitchPacketHandler: Otrzymano nieznana wartosc fazy: {}", packet->phase);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        auto result = m_stateMachine->ChangePhase(nextPhase);
        if (!result)
        {
            // Transition is not valid according to the PhaseStateMachine
            EterBase::ModernLogger::Warning("PhaseSwitchPacketHandler: Zmiana fazy odrzucona przez maszyne stanow: {}", result.error());
            return EterBase::MakeError(EterBase::PacketError::SequenceMismatch);
        }

        return {};
    }

    uint16_t PhaseSwitchPacketHandler::GetExpectedSize() const
    {
        return sizeof(TPacketGCPhase);
    }

    bool PhaseSwitchPacketHandler::IsDynamicSize() const
    {
        return false;
    }
}
