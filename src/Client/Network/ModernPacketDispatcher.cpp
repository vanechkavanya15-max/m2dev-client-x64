#include "ModernPacketDispatcher.h"

namespace Client::Network {

ModernPacketDispatcher::ModernPacketDispatcher() {
    m_handlers.fill(nullptr);
}

void ModernPacketDispatcher::RegisterHandler(uint8_t opcode, IPacketHandler* handler) {
    m_handlers[opcode] = handler;
}

void ModernPacketDispatcher::UnregisterHandler(uint8_t opcode) {
    m_handlers[opcode] = nullptr;
}

EterBase::PacketResult<void> ModernPacketDispatcher::Dispatch(uint8_t opcode, std::span<const uint8_t> payload) {
    IPacketHandler* handler = m_handlers[opcode];
    
    if (!handler) {
        return std::unexpected(EterBase::PacketError::UnknownOpcode);
    }

    if (!handler->IsDynamicSize()) {
        if (payload.size() < handler->GetExpectedSize()) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }
    } else {
        // For dynamic size, we can assume the expected size represents minimum required header/payload size
        if (payload.size() < handler->GetExpectedSize()) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }
    }

    return handler->Handle(payload);
}

} // namespace Client::Network
