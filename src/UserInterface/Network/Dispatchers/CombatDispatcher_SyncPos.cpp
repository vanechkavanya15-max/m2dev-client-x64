#include "../../StdAfx.h"
#include <cstdint>
#include <span>
#include <cstring>
#include <format>
#include <optional>
#include "../Packet.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include "../../Core/MovementEvents.h"
#include "../../EterBase/StrongTypes.h"

namespace Network::Handlers
{
    /**
     * @brief Handluje pakiet GC::SYNC_POSITION (0x0304), synchronizujac pozycje po knockbacku.
     * 
     * @param payload Surowy bufor danych reprezentujacy pakiet.
     * @return EterBase::PacketResult<void> Zwraca blad jesli ladunek jest uszkodzony.
     */
    EterBase::PacketResult<void> ProcessSyncPosition(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(TPacketGCSyncPosition))
        {
            EterBase::ModernLogger::Error("ProcessSyncPosition: Buffer underflow. Payload size {} < {}", payload.size(), sizeof(TPacketGCSyncPosition));
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCSyncPosition header = {};
        std::memcpy(&header, payload.data(), sizeof(TPacketGCSyncPosition));

        const size_t elementsDataSize = payload.size() - sizeof(TPacketGCSyncPosition);
        if (elementsDataSize % sizeof(TPacketGCSyncPositionElement) != 0)
        {
            EterBase::ModernLogger::Error("ProcessSyncPosition: Malformed payload. Leftover bytes: {}", elementsDataSize % sizeof(TPacketGCSyncPositionElement));
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        const size_t elementCount = elementsDataSize / sizeof(TPacketGCSyncPositionElement);
        EterBase::ModernLogger::Debug("ProcessSyncPosition: Received {} sync elements.", elementCount);

        const uint8_t* elementsPtr = payload.data() + sizeof(TPacketGCSyncPosition);
        for (size_t i = 0; i < elementCount; ++i)
        {
            TPacketGCSyncPositionElement element = {};
            std::memcpy(&element, elementsPtr + (i * sizeof(TPacketGCSyncPositionElement)), sizeof(TPacketGCSyncPositionElement));

            Core::Events::PlayerPositionUpdated event;
            event.entityId = EterBase::EntityId(element.dwVID);
            event.currentX = element.lX;
            event.currentY = element.lY;
            event.destinationX = element.lX; // Sync position nadpisuje current na destination
            event.destinationY = element.lY;
            event.rotation = std::nullopt; // Sync nie podaje rotacji

            if (auto result = event.Validate(); !result.has_value())
            {
                EterBase::ModernLogger::Warning("ProcessSyncPosition: Invalid event data for VID {}", element.dwVID);
                continue; // Zgodnie z zero-conflict omijamy tylko bledne elementy
            }

            EterBase::ModernLogger::Trace("ProcessSyncPosition: Broadcasting sync pos for VID {}: ({}, {})", element.dwVID, element.lX, element.lY);
            UserInterface::Core::EventBus::GetInstance().Publish(event);
        }

        return {};
    }
}
