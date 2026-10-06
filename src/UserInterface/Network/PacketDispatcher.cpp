#include "../StdAfx.h"
#include "PacketDispatcher.h"
#include "../Packet.h"

// Handlery domenowe Fali 1
#include "Handlers/DeadHandler.h"
#include "Handlers/DamageHandler.h"
#include "Handlers/TargetHPHandler.h"
#include "Handlers/ChatHandler.h"
#include "Handlers/ItemGroundDelHandler.h"
#include "Handlers/CharPositionHandler.h"

namespace Network
{
    PacketDispatcher& PacketDispatcher::Instance()
    {
        static PacketDispatcher instance;
        return instance;
    }

    void PacketDispatcher::RegisterHandler(uint16_t opcode, PacketHandlerFn handler)
    {
        m_handlers[opcode] = handler;
    }

    void PacketDispatcher::RegisterModernHandler(uint16_t opcode, ModernPacketHandlerFn handler)
    {
        m_modernHandlers[opcode] = handler;
    }

    bool PacketDispatcher::HasHandler(uint16_t opcode) const
    {
        return m_handlers.find(opcode) != m_handlers.end() || m_modernHandlers.find(opcode) != m_modernHandlers.end();
    }

    bool PacketDispatcher::Dispatch(uint16_t opcode, std::span<const uint8_t> payload)
    {
        // Sprawdz najpierw nowoczesny handler C++23
        auto modernIt = m_modernHandlers.find(opcode);
        if (modernIt != m_modernHandlers.end() && modernIt->second)
        {
            return modernIt->second(payload).has_value();
        }

        // Fallback do handlera kompatybilnego C++20
        auto it = m_handlers.find(opcode);
        if (it != m_handlers.end() && it->second)
        {
            return it->second(payload);
        }
        return false;
    }

    EterBase::PacketResult<void> PacketDispatcher::DispatchModern(uint16_t opcode, std::span<const uint8_t> payload)
    {
        auto modernIt = m_modernHandlers.find(opcode);
        if (modernIt != m_modernHandlers.end() && modernIt->second)
        {
            return modernIt->second(payload);
        }

        auto it = m_handlers.find(opcode);
        if (it != m_handlers.end() && it->second)
        {
            if (it->second(payload))
            {
                return {};
            }
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
    }

    void PacketDispatcher::RegisterDefaultHandlers()
    {
        // Rejestracja zmodernizowanych modulow domenowych C++23
        RegisterModernHandler(GC::DEAD, Handlers::ProcessDeadPacket);
        RegisterModernHandler(GC::DAMAGE_INFO, Handlers::ProcessDamagePacket);
        RegisterModernHandler(GC::TARGET, Handlers::ProcessTargetHP);
        RegisterModernHandler(GC::CHAT, Handlers::ProcessChatMessage);
        RegisterModernHandler(GC::ITEM_GROUND_DEL, Handlers::ProcessItemGroundDel);
        RegisterModernHandler(GC::CHARACTER_POSITION, Handlers::ProcessCharacterPosition);

        // Rejestracja kompatybilnych handlerow
        RegisterHandler(GC::DEAD, Handlers::HandleDeadPacket);
        RegisterHandler(GC::DAMAGE_INFO, Handlers::HandleDamagePacket);
        RegisterHandler(GC::TARGET, Handlers::HandleTargetHP);
        RegisterHandler(GC::CHAT, Handlers::HandleChatMessage);
        RegisterHandler(GC::ITEM_GROUND_DEL, Handlers::HandleItemGroundDel);
        RegisterHandler(GC::CHARACTER_POSITION, Handlers::HandleCharacterPosition);
    }
}
