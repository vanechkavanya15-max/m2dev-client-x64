#include "ModernPacketDispatcher.h"
#include "Protocol/ProtocolOpcodes.h"
#include "Protocol/Protocol.h"
#include "Handlers/CombatDamagePacketHandler.h"
#include "Handlers/ChatMessagePacketHandler.h"
#include "Handlers/ItemGroundDelHandler.h"
#include "Handlers/ItemGroundAddPacketHandler.h"
#include "Handlers/TargetHpPacketHandler.h"
#include "Handlers/StunHandler.h"
#include "Handlers/WarpTeleportPacketHandler.h"
#include "Handlers/SkillMotionPacketHandler.h"
#include "Handlers/PingPongPacketHandler.h"
#include "Handlers/AffectStatePacketHandler.h"

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

void ModernPacketDispatcher::UnregisterHandler(uint16_t opcode) {
    if (opcode < 256) {
        m_handlers[opcode] = nullptr;
    } else {
        m_extendedHandlers.erase(opcode);
    }
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

void ModernPacketDispatcher::RegisterDefaultHandlers() {
    // GC::DAMAGE_INFO (0x0410 i 0x10)
    RegisterFunctionHandler(Protocol::GC::DAMAGE_INFO, sizeof(TPacketGCDamageInfo), false,
        [](std::span<const uint8_t> payload) {
            return Handlers::CombatDamagePacketHandler::Handle(payload);
        });
    RegisterFunctionHandler(static_cast<uint16_t>(0x10), sizeof(TPacketGCDamageInfo), false,
        [](std::span<const uint8_t> payload) {
            return Handlers::CombatDamagePacketHandler::Handle(payload);
        });

    // GC::CHAT (0x0603 i 0x03)
    RegisterFunctionHandler(Protocol::GC::CHAT, sizeof(Handlers::PacketChatHeader), true,
        [](std::span<const uint8_t> payload) {
            return Handlers::ChatMessagePacketHandler::HandleNormal(payload);
        });
    RegisterFunctionHandler(static_cast<uint16_t>(0x03), sizeof(Handlers::PacketChatHeader), true,
        [](std::span<const uint8_t> payload) {
            return Handlers::ChatMessagePacketHandler::HandleNormal(payload);
        });

    // GC::WHISPER (0x0604 i 0x04)
    RegisterFunctionHandler(Protocol::GC::WHISPER, sizeof(Handlers::PacketWhisperHeader), true,
        [](std::span<const uint8_t> payload) {
            return Handlers::ChatMessagePacketHandler::HandleWhisper(payload);
        });
    RegisterFunctionHandler(static_cast<uint16_t>(0x04), sizeof(Handlers::PacketWhisperHeader), true,
        [](std::span<const uint8_t> payload) {
            return Handlers::ChatMessagePacketHandler::HandleWhisper(payload);
        });

    // GC::ITEM_GROUND_DEL (0x0516 i 0x14)
    RegisterFunctionHandler(Protocol::GC::ITEM_GROUND_DEL, sizeof(Handlers::ItemGroundDelPacket), false,
        [](std::span<const uint8_t> payload) {
            return Handlers::ItemGroundDelHandler::Handle(payload);
        });
    RegisterFunctionHandler(static_cast<uint16_t>(0x14), sizeof(Handlers::ItemGroundDelPacket), false,
        [](std::span<const uint8_t> payload) {
            return Handlers::ItemGroundDelHandler::Handle(payload);
        });

    // GC::ITEM_GROUND_ADD (0x0515 i 0x13)
    RegisterOwnedHandler(Protocol::GC::ITEM_GROUND_ADD, std::make_unique<ItemGroundAddPacketHandler>());
    RegisterOwnedHandler(static_cast<uint16_t>(0x13), std::make_unique<ItemGroundAddPacketHandler>());

    // GC::TARGET (0x0A10 i 0x18)
    RegisterFunctionHandler(Protocol::GC::TARGET, 5, false,
        [](std::span<const uint8_t> payload) {
            return Handlers::TargetHpPacketHandler::HandleTargetHpUpdate(payload);
        });
    RegisterFunctionHandler(static_cast<uint16_t>(0x18), 5, false,
        [](std::span<const uint8_t> payload) {
            return Handlers::TargetHpPacketHandler::HandleTargetHpUpdate(payload);
        });

    // GC::STUN (0x0216 i 0x1B)
    RegisterFunctionHandler(Protocol::GC::STUN, sizeof(TPacketGCStun), false,
        [](std::span<const uint8_t> payload) {
            return Handlers::HandleStunPacket(payload);
        });
    RegisterFunctionHandler(static_cast<uint16_t>(0x1B), sizeof(TPacketGCStun), false,
        [](std::span<const uint8_t> payload) {
            return Handlers::HandleStunPacket(payload);
        });

    // GC::WARP (0x0306 i 0x08)
    RegisterFunctionHandler(Protocol::GC::WARP, sizeof(TPacketGCWarp), false,
        [](std::span<const uint8_t> payload) {
            return Handlers::WarpTeleportPacketHandler::Handle(payload);
        });
    RegisterFunctionHandler(static_cast<uint16_t>(0x08), sizeof(TPacketGCWarp), false,
        [](std::span<const uint8_t> payload) {
            return Handlers::WarpTeleportPacketHandler::Handle(payload);
        });

    // GC::MOTION (0x0307 i 0x07)
    RegisterFunctionHandler(Protocol::GC::MOTION, sizeof(TPacketGCMotion), false,
        [](std::span<const uint8_t> payload) {
            return Handlers::SkillMotionPacketHandler::HandlePacket(payload);
        });
    RegisterFunctionHandler(static_cast<uint16_t>(0x07), sizeof(TPacketGCMotion), false,
        [](std::span<const uint8_t> payload) {
            return Handlers::SkillMotionPacketHandler::HandlePacket(payload);
        });

    // GC::PING (0x2C)
    RegisterFunctionHandler(static_cast<uint16_t>(0x2C), 4, false,
        [](std::span<const uint8_t> payload) {
            return Handlers::PingPongPacketHandler::HandlePingPacket(payload);
        });

    // GC::AFFECT_ADD (0x0A20) i GC::AFFECT_REMOVE (0x0A21)
    RegisterFunctionHandler(Protocol::GC::AFFECT_ADD, 12, false,
        [](std::span<const uint8_t> payload) {
            return Handlers::AffectStatePacketHandler::HandleAffectAdd(payload);
        });
    RegisterFunctionHandler(Protocol::GC::AFFECT_REMOVE, 8, false,
        [](std::span<const uint8_t> payload) {
            return Handlers::AffectStatePacketHandler::HandleAffectRemove(payload);
        });
}

void ModernPacketDispatcher::Clear() noexcept {
    m_handlers.fill(nullptr);
    m_extendedHandlers.clear();
    m_ownedHandlers.clear();
}

} // namespace Client::Network
