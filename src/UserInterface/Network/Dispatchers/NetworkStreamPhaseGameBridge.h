#pragma once

#include <span>
#include <cstdint>
#include "../PacketDispatcher.h"
#include "../Routers/PhaseGamePacketDispatcher.h"
#include "../../Core/EventBus.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"

namespace UserInterface::Core {
    struct PacketProcessingErrorEvent : public IEvent {
        uint16_t opcode;
        EterBase::PacketError error;

        PacketProcessingErrorEvent(uint16_t op, EterBase::PacketError err) 
            : opcode(op), error(err) {}
    };
}

namespace Network::Dispatchers {
    class NetworkStreamPhaseGameBridge {
    public:
        static inline EterBase::PacketResult<void> RouteGamePacket(uint16_t opcode, std::span<const uint8_t> payload) {
            EterBase::ModernLogger::Trace("NetworkStreamPhaseGameBridge: Routing packet opcode 0x{:04X}, size: {} bytes", opcode, payload.size());
            
            // Filar 2: Dyspozycja pakietu do routerow domenowych
            UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().DispatchPacket(opcode, payload.data());

            auto& dispatcher = Network::PacketDispatcher::Instance();

            if (!dispatcher.HasHandler(opcode)) {
                EterBase::ModernLogger::Trace("NetworkStreamPhaseGameBridge: Unhandled packet opcode 0x{:04X}", opcode);
                return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
            }

            auto result = dispatcher.DispatchModern(opcode, payload);
            if (!result.has_value()) {
                EterBase::ModernLogger::Error("NetworkStreamPhaseGameBridge: Failed to dispatch packet opcode 0x{:04X}. Error: {}", 
                    opcode, EterBase::ToString(result.error()));
                
                UserInterface::Core::EventBus::GetInstance().Publish(
                    UserInterface::Core::PacketProcessingErrorEvent(opcode, result.error())
                );
                return result;
            }

            EterBase::ModernLogger::Debug("NetworkStreamPhaseGameBridge: Packet opcode 0x{:04X} successfully dispatched via C++23 dispatcher.", opcode);
            return {};
        }
    };
}
