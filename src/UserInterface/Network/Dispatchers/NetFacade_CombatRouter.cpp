#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace Network::Dispatchers
{
#pragma pack(push, 1)

    // Odpowiada TPacketGCDamageInfo
    struct PacketDamageInfo
    {
        uint16_t header;
        uint16_t length;
        uint32_t victimVid;
        uint8_t damageFlag;
        int32_t damageValue;
    };

    // Odpowiada TPacketGCPVP
    struct PacketPvp
    {
        uint16_t header;
        uint16_t length;
        uint32_t srcVid;
        uint32_t dstVid;
        uint8_t mode;
    };

    // Odpowiada TPacketGCDuelStart
    struct PacketDuelStart
    {
        uint16_t header;
        uint16_t length;
    };

#pragma pack(pop)

    // Struktury zdarzeń
    struct CombatDamageReceivedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId victimId;
        uint8_t damageFlag;
        int32_t damageValue;
    };

    struct CombatPvpStateEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId srcId;
        EterBase::EntityId dstId;
        uint8_t mode;
    };

    struct CombatDuelStartEvent : public UserInterface::Core::IEvent
    {
    };

    class NetFacade_CombatRouter
    {
    public:
        static EterBase::PacketResult<void> Route(uint16_t opcode, std::span<const uint8_t> payload)
        {
            switch (opcode)
            {
            case GC::DAMAGE_INFO:
                return ProcessDamageInfo(payload);
            case GC::PVP:
                return ProcessPvp(payload);
            case GC::DUEL_START:
                return ProcessDuelStart(payload);
            default:
                EterBase::ModernLogger::Error("NetFacade_CombatRouter: Nieznany opcode {}", opcode);
                return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
            }
        }

    private:
        static EterBase::PacketResult<void> ProcessDamageInfo(std::span<const uint8_t> payload)
        {
            if (payload.size() < sizeof(PacketDamageInfo))
            {
                EterBase::ModernLogger::Error("NetFacade_CombatRouter: BufferUnderflow dla DAMAGE_INFO");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            const auto* packet = reinterpret_cast<const PacketDamageInfo*>(payload.data());
            EterBase::EntityId victimId(packet->victimVid);

            CombatDamageReceivedEvent event;
            event.victimId = victimId;
            event.damageFlag = packet->damageFlag;
            event.damageValue = packet->damageValue;

            EterBase::ModernLogger::Debug("NetFacade_CombatRouter: Received DAMAGE_INFO for victim {}", victimId.value());

            UserInterface::Core::EventBus::GetInstance().Publish(event);

            return {};
        }

        static EterBase::PacketResult<void> ProcessPvp(std::span<const uint8_t> payload)
        {
            if (payload.size() < sizeof(PacketPvp))
            {
                EterBase::ModernLogger::Error("NetFacade_CombatRouter: BufferUnderflow dla PVP");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            const auto* packet = reinterpret_cast<const PacketPvp*>(payload.data());
            EterBase::EntityId srcId(packet->srcVid);
            EterBase::EntityId dstId(packet->dstVid);

            CombatPvpStateEvent event;
            event.srcId = srcId;
            event.dstId = dstId;
            event.mode = packet->mode;

            EterBase::ModernLogger::Debug("NetFacade_CombatRouter: Received PVP state {} for src {} and dst {}", packet->mode, srcId.value(), dstId.value());

            UserInterface::Core::EventBus::GetInstance().Publish(event);

            return {};
        }

        static EterBase::PacketResult<void> ProcessDuelStart(std::span<const uint8_t> payload)
        {
            if (payload.size() < sizeof(PacketDuelStart))
            {
                EterBase::ModernLogger::Error("NetFacade_CombatRouter: BufferUnderflow dla DUEL_START");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            CombatDuelStartEvent event;

            EterBase::ModernLogger::Debug("NetFacade_CombatRouter: Received DUEL_START");

            UserInterface::Core::EventBus::GetInstance().Publish(event);

            return {};
        }
    };
}
