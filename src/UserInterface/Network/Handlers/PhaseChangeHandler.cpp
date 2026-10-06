#include "../../StdAfx.h"
#include "PhaseChangeHandler.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include <cstring>

namespace Network::Handlers
{
    /**
     * @brief Struktura zdarzenia informujaca o zmianie fazy gry.
     */
    struct PhaseChangedEvent : public UserInterface::Core::IEvent
    {
        uint8_t phase;

        /**
         * @brief Inicjalizuje strukture zdarzenia.
         * @param phase Identyfikator nowej fazy.
         */
        explicit PhaseChangedEvent(uint8_t phase) : phase(phase) {}
    };

    /**
     * @brief Przetwarza pakiet zmiany fazy z sieci.
     * @param buffer Bufor z danymi pakietu.
     * @return EterBase::PacketResult<void> wskazujacy sukces lub kod bledu PacketError.
     */
    EterBase::PacketResult<void> ProcessPhaseChangePacket(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCPhase))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCPhase packet{};
        std::memcpy(&packet, buffer.data(), sizeof(packet));

        EterBase::ModernLogger::Log(EterBase::LogLevel::Info, "Phase changed to {}", packet.phase);

        PhaseChangedEvent event{packet.phase};
        Core::EventBus::Instance().Publish(event);

        return {};
    }
}
