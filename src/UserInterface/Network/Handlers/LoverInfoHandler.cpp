#include "StdAfx.h"
#include "LoverInfoHandler.h"
#include "../../Packet.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/LogModern.h"
#include <string_view>
#include <span>
#include <cstring>
#include <string>


namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessLoverInfo(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCLoverInfo))
        {
            EterBase::ModernLogger::Error("ProcessLoverInfo: Buffer underflow (expected at least {} bytes, got {})", 
                sizeof(TPacketGCLoverInfo), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCLoverInfo*>(buffer.data());

        // Extracting name safely handling lack of null-termination if max len
        std::string_view nameView(packet->szName, strnlen(packet->szName, sizeof(packet->szName)));
        
        LoverInfoEvent event{
            std::string(nameView),
            packet->byLovePoint
        };

        EterBase::ModernLogger::Debug("ProcessLoverInfo: Name={}, LovePoint={}", event.name, event.lovePoint);

        UserInterface::Core::EventBus::Instance().Publish(event);

        return {};
    }

    bool HandleLoverInfo(std::span<const uint8_t> buffer)
    {
        return ProcessLoverInfo(buffer).has_value();
    }
}
