#include "StdAfx.h"
#include "TargetUpdateHandler.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace Network::Handlers
{
    /**
     * @brief Zdarzenie aktualizacji znacznika misji.
     * Wykorzystywane przez system EventBus do powiadomienia subskrybentow (np. MiniMapy) o zmianie.
     */
    struct TargetUpdateEvent : public UserInterface::Core::IEvent
    {
        int32_t targetId;
        int32_t x;
        int32_t y;

        explicit TargetUpdateEvent(int32_t id, int32_t _x, int32_t _y)
            : targetId(id), x(_x), y(_y) {}
    };

    EterBase::PacketResult<void> ProcessTargetUpdate(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketTargetUpdate))
        {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "TargetUpdate: Buffer underflow (size: {} < expected: {})", buffer.size(), sizeof(PacketTargetUpdate));
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketTargetUpdate*>(buffer.data());

        // Dekouplowane powiadomienie przez EventBus zamiast bezposrednich zaleznosci od GUI
        UserInterface::Core::EventBus::GetInstance().Publish(TargetUpdateEvent(packet->targetId, packet->x, packet->y));

        EterBase::ModernLogger::Log(EterBase::LogLevel::Trace, "TargetUpdate: Processed update for TargetID: {} to ({}, {})", packet->targetId, packet->x, packet->y);

        return {};
    }

    bool HandleTargetUpdate(std::span<const uint8_t> buffer)
    {
        return ProcessTargetUpdate(buffer).has_value();
    }
}
