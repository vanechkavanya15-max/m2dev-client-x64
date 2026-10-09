#include "BeaviumProtocolDriver.h"
#include "../BeaviumProtocol.h"
#include "UserInterface/Contracts/IGameEvents.h"
#include "UserInterface/Network/Routers/PhaseGamePacketDispatcher.h"
#include "EterBase/EventBus.h"
#include <cstring>

#pragma pack(push, 1)
/**
 * @brief Struktura proxy dla ataku Beavium (8 bajtow).
 */
struct ProxyPacketCGAttack
{
    uint8_t  header;     ///< Beavium::CG::ATTACK = 0x02
    uint8_t  attackType; ///< Typ ataku
    uint32_t targetVid;  ///< Virtual ID celu
    uint16_t sequence;   ///< Sekwencja / CRC
};
static_assert(sizeof(ProxyPacketCGAttack) == 8, "ProxyPacketCGAttack musi miec dokladnie 8 bajtow");
#pragma pack(pop)

namespace Network::Protocol::Drivers
{
    std::optional<FrameHeaderInfo> BeaviumProtocolDriver::InspectFrame(
        std::span<const uint8_t> buffer) const noexcept
    {
        // Wymagany co najmniej 1 bajt naglowka
        if (buffer.empty())
        {
            return std::nullopt;
        }

        uint8_t header = buffer[0];
        Beavium::BeaviumGCInfo info{};
        if (!Beavium::GetGCInfo(header, info))
        {
            return std::nullopt;
        }

        if (!info.isDynamic)
        {
            // Pakiet o stalym rozmiarze ze statycznej tablicy beavium_gc_table.inl
            return FrameHeaderInfo{
                .unifiedOpcode = static_cast<uint16_t>(header),
                .packetLength = info.size,
                .headerSize = 1
            };
        }
        else
        {
            // Pakiet dynamiczny - pole dlugosci 16-bit na offsetcie 1
            if (buffer.size() < 3)
            {
                return std::nullopt;
            }

            uint32_t dynLength = static_cast<uint32_t>(buffer[1] | (static_cast<uint32_t>(buffer[2]) << 8));
            if (dynLength < 3 || dynLength > 65000)
            {
                return std::nullopt;
            }

            return FrameHeaderInfo{
                .unifiedOpcode = static_cast<uint16_t>(header),
                .packetLength = dynLength,
                .headerSize = 1
            };
        }
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeAttack(
        const Domain::AttackCommand& cmd) const
    {
        ProxyPacketCGAttack packet{};
        packet.header = Beavium::CG::ATTACK; // 0x02
        packet.attackType = cmd.attackType;
        packet.targetVid = cmd.targetVid;
        packet.sequence = cmd.sequence;

        std::vector<uint8_t> buffer(sizeof(ProxyPacketCGAttack));
        std::memcpy(buffer.data(), &packet, sizeof(ProxyPacketCGAttack));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeMove(
        const Domain::MoveCommand& cmd) const
    {
        Beavium::TPacketCGMoveBeavium packet{};
        packet.header = Beavium::CG::MOVE; // 0x07
        packet.bFunc = cmd.func;
        packet.wArg = cmd.arg;
        packet.dwRot = Beavium::EncodeRotationMicrodegrees(cmd.rotationDegrees);
        packet.lX = cmd.x;
        packet.lY = cmd.y;
        packet.dwTime = cmd.time;
        packet.dwExtra = 0;

        std::vector<uint8_t> buffer(sizeof(Beavium::TPacketCGMoveBeavium));
        std::memcpy(buffer.data(), &packet, sizeof(Beavium::TPacketCGMoveBeavium));
        return buffer;
    }

    bool BeaviumProtocolDriver::DispatchInbound(
        uint16_t unifiedOpcode,
        std::span<const uint8_t> payload,
        UserInterface::Contracts::IGameEventSink* pSink)
    {
        if (payload.empty())
        {
            return false;
        }

        uint8_t bHeader = static_cast<uint8_t>(unifiedOpcode & 0xFF);

        // 1. Publikacja ogolnego zdarzenia odbioru ramki w magistrali EventBus
        ::EterBase::EventBus::GetInstance().Publish(
            ::EterBase::NetworkPacketReceivedEvent(bHeader, payload));

        // 2. Mapowanie kodow GC Beavium na dedykowane zdarzenia domenowe
        switch (bHeader)
        {
        case Beavium::GC::ATTACK: // 0x0C
            if (payload.size() >= sizeof(Beavium::TPacketGCAttackBeavium))
            {
                const auto& atk = *reinterpret_cast<const Beavium::TPacketGCAttackBeavium*>(payload.data());
                if (pSink)
                {
                    pSink->OnAttackExecuted({
                        .dwAttackerVID = atk.dwVID,
                        .dwVictimVID = atk.dwVictimVID,
                        .byMotionType = atk.bType
                    });
                }
            }
            break;

        case Beavium::GC::DEAD: // 0x0E
            if (payload.size() >= sizeof(Beavium::TPacketGCDeadBeavium))
            {
                const auto& dead = *reinterpret_cast<const Beavium::TPacketGCDeadBeavium*>(payload.data());
                if (pSink)
                {
                    pSink->OnActorDead({
                        .dwVID = dead.dwVID
                    });
                }
                ::EterBase::EventBus::GetInstance().Publish(::EterBase::ActorDeadEvent(dead.dwVID));
            }
            break;

        case Beavium::GC::CHARACTER_MOVE: // 0x03
            if (payload.size() >= sizeof(Beavium::TPacketGCCharacterMoveBeavium))
            {
                const auto& mv = *reinterpret_cast<const Beavium::TPacketGCCharacterMoveBeavium*>(payload.data());
                if (pSink)
                {
                    pSink->OnActorMoved({
                        .dwVID = mv.dwVID,
                        .lX = mv.lX,
                        .lY = mv.lY,
                        .fRot = static_cast<float>(static_cast<double>(mv.dwRot) / 1000000.0),
                        .dwTime = mv.dwTime
                    });
                }
            }
            break;

        case Beavium::GC::TARGET_HP: // 0x6C
            if (payload.size() >= sizeof(Beavium::TPacketGCTargetHPBeavium))
            {
                const auto& hp = *reinterpret_cast<const Beavium::TPacketGCTargetHPBeavium*>(payload.data());
                if (pSink)
                {
                    pSink->OnTargetChanged({
                        .dwVID = hp.dwTargetVID
                    });
                }
            }
            break;

        case 0x10: // GC::ITEM_SET / CHARACTER_UPDATE (82B)
            if (payload.size() >= sizeof(Beavium::TPacketGCItemSetBeavium))
            {
                const auto& item = *reinterpret_cast<const Beavium::TPacketGCItemSetBeavium*>(payload.data());
                if (pSink)
                {
                    pSink->OnItemReceived({
                        .dwCell = item.Cell,
                        .dwVnum = item.vnum,
                        .dwCount = item.count
                    });
                }
            }
            break;

        default:
            break;
        }

        // 3. Delegacja do dyspozytora pakietow PhaseGamePacketDispatcher z walidacja dlugosci bufora
        UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().DispatchPacket(
            bHeader, payload.data(), payload.size());

        return true;
    }
}
