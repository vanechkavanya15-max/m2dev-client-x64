#include "ModernPacketDispatcher.h"

namespace Client::Network {

ModernPacketDispatcher& ModernPacketDispatcher::Instance() noexcept {
    static ModernPacketDispatcher s_instance;
    return s_instance;
}

ModernPacketDispatcher::ModernPacketDispatcher() {
    m_handlers.fill(nullptr);
}

void ModernPacketDispatcher::RegisterHandler(uint16_t opcode, IPacketHandler* handler) {
    if (opcode < 256) {
        m_handlers[opcode] = handler;
    } else {
        m_extendedHandlers[opcode] = handler;
    }
}

void ModernPacketDispatcher::RegisterHandler(uint8_t opcode, IPacketHandler* handler) {
    m_handlers[opcode] = handler;
}

void ModernPacketDispatcher::UnregisterHandler(uint16_t opcode) {
    if (opcode < 256) {
        m_handlers[opcode] = nullptr;
    } else {
        m_extendedHandlers.erase(opcode);
    }
}

void ModernPacketDispatcher::UnregisterHandler(uint8_t opcode) {
    m_handlers[opcode] = nullptr;
}

bool ModernPacketDispatcher::HasHandler(uint16_t opcode) const noexcept {
    if (opcode < 256) {
        return m_handlers[opcode] != nullptr;
    }
    return m_extendedHandlers.find(opcode) != m_extendedHandlers.end();
}

void ModernPacketDispatcher::RegisterOwnedHandler(uint16_t opcode, std::unique_ptr<IPacketHandler> handler) {
    if (!handler) return;
    IPacketHandler* ptr = handler.get();
    m_ownedHandlers.push_back(std::move(handler));
    RegisterHandler(opcode, ptr);
}

void ModernPacketDispatcher::RegisterOwnedHandler(uint8_t opcode, std::unique_ptr<IPacketHandler> handler) {
    RegisterOwnedHandler(static_cast<uint16_t>(opcode), std::move(handler));
}

EterBase::PacketResult<void> ModernPacketDispatcher::Dispatch(uint16_t opcode, std::span<const uint8_t> payload) {
    IPacketHandler* handler = nullptr;
    if (opcode < 256) {
        handler = m_handlers[opcode];
    } else {
        auto it = m_extendedHandlers.find(opcode);
        if (it != m_extendedHandlers.end()) {
            handler = it->second;
        }
    }
    
    if (!handler) {
        return std::unexpected(EterBase::PacketError::UnknownOpcode);
    }

    if (payload.size() < handler->GetExpectedSize()) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    return handler->Handle(payload);
}

EterBase::PacketResult<void> ModernPacketDispatcher::Dispatch(uint8_t opcode, std::span<const uint8_t> payload) {
    return Dispatch(static_cast<uint16_t>(opcode), payload);
}

void ModernPacketDispatcher::Clear() noexcept {
    m_handlers.fill(nullptr);
    m_extendedHandlers.clear();
    m_ownedHandlers.clear();
}

} // namespace Client::Network
