#pragma once

#include <span>
#include <cstdint>
#include "../Routers/PhaseGamePacketDispatcher.h"
#include "../../Core/EventBus.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"

namespace Network::Dispatchers {
    class NetworkStreamPhaseGameBridge {
    public:
        static inline EterBase::PacketResult<void> RouteGamePacket(uint16_t opcode, std::span<const uint8_t> payload) {
            EterBase::ModernLogger::Trace("NetworkStreamPhaseGameBridge: Routing packet opcode 0x{:04X}, size: {} bytes", opcode, payload.size());
            
            // Deterministyczna dyspozycja O(1) bezposrednio przez centralny PhaseGamePacketDispatcher
            if (UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().RouteGamePacket(opcode, payload.data(), payload.size())) {
                return {};
            }

            EterBase::ModernLogger::Trace("NetworkStreamPhaseGameBridge: Unhandled packet opcode 0x{:04X}", opcode);
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }
    };
}
